/*
    Example source code for "Securing Dedicated Servers"

    Copyright © 2016, The Network Protocol Company, Inc.
    
    All rights reserved.

    Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

        1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

        2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
           in the documentation and/or other materials provided with the distribution.

        3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived 
           from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
    INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
    SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
    SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
    WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
    USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "yojimbo.h"
#include <sodium.h>
#include <stdio.h>
#include <stdlib.h>

using namespace yojimbo;
using namespace protocol2;
using namespace network2;

const uint32_t ProtocolId = 0x12341651;

const int ServerPort = 50000;

const int NonceBytes = crypto_aead_chacha20poly1305_NPUBBYTES;
const int KeyBytes = crypto_aead_chacha20poly1305_KEYBYTES;
const int AuthBytes = crypto_aead_chacha20poly1305_ABYTES;
const int MacBytes = crypto_secretbox_MACBYTES;
const int TokenBytes = 1024;
const int MaxServersPerToken = 8;
const int TokenExpirySeconds = 10;

struct Token
{
    uint32_t protocol_id;                                               // the protocol id this token belongs to
    uint64_t client_id;                                                 // the unique client id. max one connection per-client per-server.
    uint64_t expiry_timestamp;                                          // timestamp this token expires (eg. 10 seconds after token creation)
    int num_server_addresses;                                           // the number of server addresses this token may be used on
    Address server_address[MaxServersPerToken];                         // token only works with this list of server addresses.
    uint8_t nonce[NonceBytes];                                          // the nonce for encrypted communication with this client post-connection.
    uint8_t key[KeyBytes];                                              // the key for encryption communication with this client post-connection

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_uint32( stream, protocol_id );

        serialize_uint64( stream, client_id );
        
        serialize_uint64( stream, expiry_timestamp );
        
        serialize_int( stream, num_server_addresses, 0, MaxServersPerToken - 1 );
        
        for ( int i = 0; i < num_server_addresses; ++i )
        {
            char buffer[64];
            if ( Stream::IsWriting )
            {
                assert( server_address[i].IsValid() );
                server_address[i].ToString( buffer, sizeof( buffer ) );
            }
            serialize_string( stream, buffer, sizeof( buffer ) );
            if ( Stream::IsReading )
            {
                server_address[i] = Address( buffer );
                if ( !server_address[i].IsValid() )
                    return false;
            }
        }

        serialize_bytes( stream, nonce, NonceBytes );

        serialize_bytes( stream, key, KeyBytes );

        return true;
    }

    bool operator == ( const Token & other ) const
    {
        if ( protocol_id != other.protocol_id )
            return false;

        if ( client_id != other.client_id )
            return false;

        if ( expiry_timestamp != other.expiry_timestamp )
            return false;

        if ( num_server_addresses != other.num_server_addresses )
            return false;

        for ( int i = 0; i < num_server_addresses; ++i )
        {
            if ( server_address[i] != other.server_address[i] )
                return false;
        }

        return true;
    }

    bool operator != ( const Token & other ) const
    {
        return !( (*this)== other );
    }
};

void GenerateToken( Token & token, uint64_t clientId, int numServerAddresses, const Address * serverAddresses )
{
    uint64_t timestamp = (uint64_t) time( NULL );

    token.protocol_id = ProtocolId;
    token.client_id = clientId;
    token.expiry_timestamp = timestamp + TokenExpirySeconds;
    
    assert( numServerAddresses > 0 );
    assert( numServerAddresses <= MaxServersPerToken );
    token.num_server_addresses = numServerAddresses;
    for ( int i = 0; i < numServerAddresses; ++i )
        token.server_address[i] = serverAddresses[i];

    randombytes_buf( token.key, sizeof( token.key ) );
    randombytes_buf( token.nonce, sizeof( token.nonce ) );
}

bool EncryptToken( Token & token, uint8_t *encryptedMessage, const uint8_t *additional, int additionalLength, const uint8_t *nonce, const uint8_t *key )
{
    uint8_t message[TokenBytes];
    memset( message, 0, TokenBytes );
    WriteStream stream( message, TokenBytes );
    if ( !token.Serialize( stream ) )
        return false;

    stream.Flush();
    
    if ( stream.GetError() )
        return false;

    unsigned long long encryptedLength;

    const int messageLength = TokenBytes;

    int result = crypto_aead_chacha20poly1305_encrypt( encryptedMessage, &encryptedLength,
                                                       message, messageLength,
                                                       additional, additionalLength,
                                                       NULL, nonce, key );

    if ( result != 0 )
        return false;

    assert( encryptedLength == TokenBytes + AuthBytes );

    return true;
}

bool DecryptToken( const uint8_t * encryptedMessage, Token & decryptedToken, const uint8_t * additional, int additionalLength, const uint8_t * nonce, const uint8_t * key )
{
    unsigned char decryptedMessage[TokenBytes];
    
    unsigned long long decryptedMessageLength;
    
    const int encryptedMessageLength = TokenBytes + AuthBytes;

    int result = crypto_aead_chacha20poly1305_decrypt( decryptedMessage, &decryptedMessageLength,
                                                       NULL,
                                                       encryptedMessage, encryptedMessageLength,
                                                       additional, additionalLength,
                                                       nonce, key );

    assert( decryptedMessageLength == TokenBytes );

    if ( result != 0 )
        return false;

    ReadStream stream( decryptedMessage, TokenBytes );
    if ( !decryptedToken.Serialize( stream ) )
        return false;

    if ( stream.GetError() )
        return false;

    return true;
}

bool EncryptPacket( const uint8_t * packetData, int packetLength, uint8_t *encryptedPacketData, int & encryptedPacketLength, const uint8_t *nonce, const uint8_t *key )
{
    if ( crypto_secretbox_easy( encryptedPacketData, packetData, packetLength, nonce, key ) != 0 )
        return false;

    encryptedPacketLength = packetLength + MacBytes;

    return true;
}

bool DecryptPacket( const uint8_t * encryptedPacketData, int encryptedPacketLength, uint8_t *decryptedPacketData, int & decryptedPacketLength, const uint8_t *nonce, const uint8_t *key )
{
    if ( crypto_secretbox_open_easy( decryptedPacketData, encryptedPacketData, encryptedPacketLength, nonce, key ) != 0 )
        return false;

    decryptedPacketLength = encryptedPacketLength - MacBytes;

    return true;
}

enum PacketTypes
{
    PACKET_CONNECTION_REQUEST,                      // client requests a connection.
    PACKET_CONNECTION_DENIED,                       // server denies client connection request.
    PACKET_CONNECTION_CHALLENGE,                    // server response to client connection request.
    PACKET_CONNECTION_RESPONSE,                     // client response to server connection challenge.
    PACKET_CONNECTION_KEEP_ALIVE,                   // keep alive packet sent at some low rate (once per-second) to keep the connection alive
    PACKET_CONNECTION_DISCONNECT,                   // courtesy packet to indicate that the other side has disconnected. better than a timeout
    CLIENT_SERVER_NUM_PACKETS
};

struct ConnectionRequestPacket : public Packet
{
    uint8_t token_data[TokenBytes+AuthBytes];       // encrypted token data generated by matchmaker
    uint8_t token_salt[NonceBytes];                 // salt value required to decrypt the token on the server

    ConnectionRequestPacket() : Packet( PACKET_CONNECTION_REQUEST )
    {
        memset( token_data, 0, sizeof( token_data ) );
        memset( token_salt, 0, sizeof( token_salt ) );
    }

    template <typename Stream> bool Serialize( Stream & stream )
    {
        serialize_bytes( stream, token_data, sizeof( token_data ) );
        serialize_bytes( stream, token_salt, sizeof( token_salt ) );
        return true;
    }

    PROTOCOL2_DECLARE_VIRTUAL_SERIALIZE_FUNCTIONS();
};

int main()
{
    if ( sodium_init() == -1 )
    {
        printf( "error: failed to initialize libsodium\n" );
        return 1;
    }

    Token token;

    Address serverAddress( "::1", ServerPort );

    GenerateToken( token, 1231241, 1, &serverAddress );

    uint8_t encryptedToken[TokenBytes+AuthBytes];
    uint8_t nonce[NonceBytes];
    uint8_t key[KeyBytes];

    randombytes_buf( key, sizeof( key ) );
    randombytes_buf( nonce, sizeof( nonce ) );

    if ( !EncryptToken( token, encryptedToken, NULL, 0, nonce, key ) )
    {
        printf( "error: failed to encrypt token\n" );
        return 1;
    }

    printf( "successfully encrypted token\n" );

    Token decryptedToken;
    if ( !DecryptToken( encryptedToken, decryptedToken, NULL, 0, nonce, key ) )
    {
        printf( "error: failed to decrypt token\n" );
        return 1;
    }

    printf( "successfully decrypted token\n" );

    if ( decryptedToken == token )
    {
        printf( "success: decrypted token matches original token!\n" );
    }
    else
    {
        printf( "error: decrypted token does not match original token\n" );
        return 1;
    }

    uint8_t packet[1024];

    randombytes_buf( packet, sizeof( packet ) );

    uint8_t encryptedPacket[2048];
    int encryptedPacketLength;
    if ( !EncryptPacket( packet, sizeof( packet ), encryptedPacket, encryptedPacketLength, token.nonce, token.key ) )
    {
        printf( "error: failed to encrypt packet\n" );
        return 1;
    }

    printf( "successfully encrypted packet\n" );

    uint8_t decryptedPacket[2048];
    int decryptedPacketLength;
    if ( !DecryptPacket( encryptedPacket, encryptedPacketLength, decryptedPacket, decryptedPacketLength, token.nonce, token.key ) )
    {
        printf( "error: failed to decrypt packet\n" );
        return 1;
    }

    printf( "successfull decrypted packet\n" );

    if ( decryptedPacketLength == sizeof( packet ) && memcmp( packet, decryptedPacket, sizeof( packet ) ) == 0 )
    {
        printf( "success: decrypted packet matches original packet!\n" );

    }
    else
    {
        printf( "error: decrypted packet does not match original packet\n" );
        return 1;
    }

    return 0;
}