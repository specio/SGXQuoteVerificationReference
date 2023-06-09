/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "OpensslHelpers/OidUtils.h"
#include "ParserUtils.h"
#include "Utils/Logger.h"

namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {

ProcessorPckCertificate::ProcessorPckCertificate(const Certificate& certificate): PckCertificate(certificate)
{
    auto sgxExtensions = crypto::make_unique(getSgxExtensions());
    setMembers(sgxExtensions.get());
}

ProcessorPckCertificate ProcessorPckCertificate::parse(const std::string& pem)
{
    return ProcessorPckCertificate(pem);
}

// Private

ProcessorPckCertificate::ProcessorPckCertificate(const std::string& pem): PckCertificate(pem)
{
    auto sgxExtensions = crypto::make_unique(getSgxExtensions());
    setMembers(sgxExtensions.get());
}


void ProcessorPckCertificate::setMembers(stack_st_ASN1_TYPE *sgxExtensions)
{
    PckCertificate::setMembers(sgxExtensions);

    const auto stackEntries = sk_ASN1_TYPE_num(sgxExtensions);
    if(stackEntries != PROCESSOR_CA_EXTENSION_COUNT)
    {
        std::string err = "OID [" + oids::SGX_EXTENSION + "] expected to contain [" + std::to_string(PROCESSOR_CA_EXTENSION_COUNT) +
                          "] elements when given [" + std::to_string(stackEntries) + "]";
        LOG_AND_THROW(InvalidExtensionException, err);
    }
}

}}}}} // namespace intel { namespace sgx { namespace dcap { namespace parser { namespace x509 {
