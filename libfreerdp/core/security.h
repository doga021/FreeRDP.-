/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * RDP Security
 *
 * Copyright 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __SECURITY_H
#define __SECURITY_H

#include "rdp.h"
#include <freerdp/crypto/crypto.h>

#include <freerdp/freerdp.h>
#include <freerdp/utils/stream.h>

void security_master_secret(BYTE* premaster_secret, BYTE* client_random, BYTE* server_random, BYTE* output);
void security_session_key_blob(BYTE* master_secret, BYTE* client_random, BYTE* server_random, BYTE* output);
void security_mac_salt_key(BYTE* session_key_blob, BYTE* client_random, BYTE* server_random, BYTE* output);
void security_licensing_encryption_key(BYTE* session_key_blob, BYTE* client_random, BYTE* server_random, BYTE* output);
void security_mac_data(BYTE* mac_salt_key, BYTE* data, uint32 length, BYTE* output);

void security_mac_signature(rdpRdp *rdp, BYTE* data, uint32 length, BYTE* output);
void security_salted_mac_signature(rdpRdp *rdp, BYTE* data, uint32 length, BOOL encryption, BYTE* output);
BOOL security_establish_keys(BYTE* client_random, rdpRdp* rdp);

BOOL security_encrypt(BYTE* data, int length, rdpRdp* rdp);
BOOL security_decrypt(BYTE* data, int length, rdpRdp* rdp);

void security_hmac_signature(BYTE* data, int length, BYTE* output, rdpRdp* rdp);
BOOL security_fips_encrypt(BYTE* data, int length, rdpRdp* rdp);
BOOL security_fips_decrypt(BYTE* data, int length, rdpRdp* rdp);
BOOL security_fips_check_signature(BYTE* data, int length, BYTE* sig, rdpRdp* rdp);

#endif /* __SECURITY_H */
