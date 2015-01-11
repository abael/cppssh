/*
    cppssh - C++ ssh library
    Copyright (C) 2015  Chris Desjardins
    http://blog.chrisd.info cjd@chrisd.info

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "packet.h"

#if !defined(WIN32) && !defined(__MINGW32__)
#   include <arpa/inet.h>
#else
#   include <Winsock2.h>
#endif

#define NE7SSH_PACKET_LENGTH_OFFS   0
#define NE7SSH_PACKET_LENGTH_SIZE   4

#define NE7SSH_PACKET_PAD_OFFS      4
#define NE7SSH_PACKET_PAD_SIZE      1

#define NE7SSH_PACKET_PAYLOAD_OFFS  5
#define NE7SSH_PACKET_CMD_SIZE      1

CppsshPacket::CppsshPacket(Botan::secure_vector<Botan::byte> *data)
    : _data(data)
{

}

void CppsshPacket::addVectorField(const Botan::secure_vector<Botan::byte> &vec)
{
    addInt(vec.size());
    (*_data) += vec;
}

void CppsshPacket::addVector(const Botan::secure_vector<Botan::byte> &vec)
{
    (*_data) += vec;
}

void CppsshPacket::addString(const std::string& str)
{
    addInt(str.length());
    (*_data) += Botan::secure_vector<Botan::byte>(str.begin(), str.end());
}

void CppsshPacket::addInt(const uint32_t var)
{
    uint32_t data = htonl(var);
    Botan::byte *p;
    p = (Botan::byte *)&data;
    (*_data).push_back(p[0]);
    (*_data).push_back(p[1]);
    (*_data).push_back(p[2]);
    (*_data).push_back(p[3]);
}

void CppsshPacket::addChar(const char ch)
{
    (*_data).push_back(ch);
}

void CppsshPacket::addBigInt(const Botan::BigInt& bn)
{
    Botan::secure_vector<Botan::byte> converted;
    bn2vector(converted, bn);
    addVectorField(converted);
}

void CppsshPacket::bn2vector(Botan::secure_vector<Botan::byte>& result, const Botan::BigInt& bi)
{
    bool high;

    std::vector<Botan::byte> strVector = Botan::BigInt::encode(bi);

    high = (*(strVector.begin()) & 0x80) ? true : false;

    if (high == true)
    {
        result.push_back(0);
    }
    else
    {
        result.clear();
    }
    result += strVector;
}

CppsshPacket& CppsshPacket::operator=(Botan::secure_vector<Botan::byte> *encryptedPacket)
{
    _data = encryptedPacket;
    return *this;
}

uint32_t CppsshPacket::getPacketLength()
{
    uint32_t ret = 0;
    if (_data->size() >= NE7SSH_PACKET_LENGTH_SIZE)
    {
        ret = ntohl(*((uint32_t*)_data->data()));
    }
    return ret;
}

uint32_t CppsshPacket::getCryptoLength()
{
    uint32_t ret = getPacketLength();
    if (ret > 0)
    {
        ret += sizeof(uint32_t);
    }
    return ret;
}

Botan::byte CppsshPacket::getPadLength()
{
    Botan::byte ret = 0;
    if (_data->size() >= (NE7SSH_PACKET_PAD_OFFS + NE7SSH_PACKET_PAD_SIZE))
    {
        ret = _data->begin()[NE7SSH_PACKET_PAD_OFFS];
    }
    return ret;
}

Botan::byte CppsshPacket::getCommand()
{
    Botan::byte ret = 0;
    if (_data->size() >= (NE7SSH_PACKET_PAYLOAD_OFFS + NE7SSH_PACKET_CMD_SIZE))
    {
        ret = _data->begin()[NE7SSH_PACKET_PAYLOAD_OFFS];
    }
    return ret;
}

Botan::byte* CppsshPacket::getPayload()
{
    Botan::byte* ret = NULL;
    if (_data->size() > NE7SSH_PACKET_PAYLOAD_OFFS)
    {
        ret = _data->data() + NE7SSH_PACKET_PAYLOAD_OFFS;
    }
    return ret;
}

bool CppsshPacket::getString(Botan::secure_vector<Botan::byte>& result)
{
    bool ret = true;
    uint32_t len = getPacketLength();

    if (len > _data->size())
    {
        ret = false;
    }
    else
    {
        result = Botan::secure_vector<Botan::byte>(_data->begin() + sizeof(uint32_t), _data->begin() + (sizeof(uint32_t) + len));
        _data->erase(_data->begin(), _data->begin() + (sizeof(uint32_t) + len));
    }
    return ret;
}