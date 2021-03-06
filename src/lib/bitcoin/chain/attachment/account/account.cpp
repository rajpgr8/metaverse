/**
 * Copyright (c) 2011-2015 metaverse developers (see AUTHORS)
 *
 * This file is part of mvs-node.
 *
 * metaverse is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <metaverse/bitcoin/chain/attachment/account/account.hpp>

#include <sstream>
#include <boost/iostreams/stream.hpp>
#include <metaverse/bitcoin/utility/container_sink.hpp>
#include <metaverse/bitcoin/utility/container_source.hpp>
#include <metaverse/bitcoin/utility/istream_reader.hpp>
#include <metaverse/bitcoin/utility/ostream_writer.hpp>

#ifdef MVS_DEBUG
#include <json/minijson_writer.hpp>
#endif

#include <metaverse/bitcoin/math/crypto.hpp>
#include <metaverse/bitcoin.hpp>
using namespace libbitcoin::wallet;

namespace libbitcoin {
namespace chain {

account::account()
{
	reset();
}
account::account(std::string name, std::string mnemonic, hash_digest passwd, 
		uint32_t hd_index, uint8_t priority, uint16_t status):
		name(name), mnemonic(mnemonic), passwd(passwd), hd_index(hd_index), priority(priority), status(status)
{
}

account account::factory_from_data(const data_chunk& data)
{
    account instance;
    instance.from_data(data);
    return instance;
}

account account::factory_from_data(std::istream& stream)
{
    account instance;
    instance.from_data(stream);
    return instance;
}

account account::factory_from_data(reader& source)
{
    account instance;
    instance.from_data(source);
    return instance;
}

bool account::is_valid() const
{
    return true;
}

void account::reset()
{	
    this->name = "";
    this->mnemonic = "";
    //this->passwd = "";
    this->hd_index = 0;
    this->priority = account_priority::common_user; // 0 -- admin user  1 -- common user
    this->status = account_status::normal;
}

bool account::from_data(const data_chunk& data)
{
    data_source istream(data);
    return from_data(istream);
}

bool account::from_data(std::istream& stream)
{
    istream_reader source(stream);
    return from_data(source);
}

bool account::from_data(reader& source)
{
    reset();
    name = source.read_string();
    //mnemonic = source.read_string();
    
	// read encrypted mnemonic
	auto size = source.read_variable_uint_little_endian();
	data_chunk string_bytes = source.read_data(size);
	std::string result(string_bytes.begin(), string_bytes.end());
	mnemonic = result;
	
    passwd = source.read_hash();
    hd_index= source.read_4_bytes_little_endian();
    priority= source.read_byte();
	status = source.read_2_bytes_little_endian();
    return true;	
}

data_chunk account::to_data() const
{
    data_chunk data;
    data_sink ostream(data);
    to_data(ostream);
    ostream.flush();
    //BITCOIN_ASSERT(data.size() == serialized_size()); // serialized_size is not used
    return data;
}

void account::to_data(std::ostream& stream) const
{
    ostream_writer sink(stream);
    to_data(sink);
}

void account::to_data(writer& sink) const
{
    sink.write_string(name);
	sink.write_string(mnemonic);
	sink.write_hash(passwd);
	sink.write_4_bytes_little_endian(hd_index);
	sink.write_byte(priority);
	sink.write_2_bytes_little_endian(status);
}

uint64_t account::serialized_size() const
{
    return name.size() + mnemonic.size() + passwd.size() + 4 + 1 + 3; // 2 "string length" byte
}

account::operator bool() const
{
	return (name.empty() || mnemonic.empty());
}
bool account::operator==(const account& other) const
{
    return (name.compare(other.get_name()) == 0);
			//&& (mnemonic.compare(other.get_mnemonic()) == 0); // mnemonic not equal when account change passwd
}


#ifdef MVS_DEBUG
std::string account::to_string() 
{
    std::ostringstream ss;

    ss << "\t name = " << name << "\n"
		<< "\t mnemonic = " << mnemonic << "\n"
		<< "\t password = " << passwd.data() << "\n"
		<< "\t hd_index = " << hd_index << "\n"
		<< "\t priority = " << priority << "\n"
		<< "\t status = " << status << "\n";

    return ss.str();
}
void account::to_json(std::ostream& output) 
{
	minijson::object_writer json_writer(output);
	json_writer.write("name", name);
	json_writer.write("mnemonic", mnemonic);
	//json_writer.write_array("passwd", std::begin(passwd.data()), std::passwd.end());
	json_writer.write("hd_index", hd_index);
	json_writer.write("priority", priority);
	json_writer.close();
}
#endif

const std::string& account::get_name() const
{ 
    return name;
}
void account::set_name(const std::string& name)
{ 
     this->name = name;
}
const std::string& account::get_mnemonic() const
{
	return mnemonic; // for account == operator
}
const std::string& account::get_mnemonic(std::string& passphrase, std::string& decry_output) const
{
	decrypt_string(mnemonic, passphrase, decry_output);
	return decry_output;
}

void account::set_mnemonic(const std::string& mnemonic, std::string& passphrase)
{ 
	if(!mnemonic.size())
		throw std::logic_error{"mnemonic size is 0"};
	if(!passphrase.size())
		throw std::logic_error{"invalid password!"};
	std::string encry_output("");

	encrypt_string(mnemonic, passphrase, encry_output);
	this->mnemonic = encry_output;
}

const hash_digest& account::get_passwd() const
{ 
    return passwd;
}

uint32_t account::get_hd_index() const
{ 
    return hd_index;
}
void account::set_hd_index(uint32_t hd_index)
{ 
     this->hd_index = hd_index;
}

uint16_t account::get_status() const
{ 
    return status;
}
void account::set_status(uint16_t status)
{ 
     this->status = status;
}

uint8_t account::get_priority() const
{ 
    return priority;
}
void account::set_priority(uint8_t priority)
{ 
     this->priority = priority;
}

void account::set_user_status(uint8_t status)
{ 
     this->status |= status;
}
void account::set_system_status(uint8_t status)
{ 
	this->status |= (status<<sizeof(uint8_t));
}
uint8_t account::get_user_status() const
{ 
     return static_cast<uint8_t>(status&0xff);
}
uint8_t account::get_system_status() const
{ 
	return static_cast<uint8_t>(status>>sizeof(uint8_t));
}

} // namspace chain
} // namspace libbitcoin
