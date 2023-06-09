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

#include "EnclaveIdentityVerifier.h"
#include "EnclaveIdentityV2.h"
#include "Utils/TimeUtils.h"
#include <Utils/Logger.h>

#include <CertVerification/X509Constants.h>
#include <OpensslHelpers/SignatureVerification.h>

namespace intel { namespace sgx { namespace dcap {

EnclaveIdentityVerifier::EnclaveIdentityVerifier()
        : _commonVerifier(new CommonVerifier()),
          _tcbSigningChain(new TCBSigningChain())
{
}

EnclaveIdentityVerifier::EnclaveIdentityVerifier(std::unique_ptr<CommonVerifier>&& commonVerifier,
                                 std::unique_ptr<TCBSigningChain>&& tcbSigningChain)
        : _commonVerifier(std::move(commonVerifier)), _tcbSigningChain(std::move(tcbSigningChain))
{
}

Status EnclaveIdentityVerifier::verify(
            const EnclaveIdentityV2 &enclaveIdentity,
            const CertificateChain &chain,
            const pckparser::CrlStore &rootCaCrl,
            const dcap::parser::x509::Certificate &trustedRoot,
            const std::time_t& expirationDate) const
{
    const auto status = _tcbSigningChain->verify(chain, rootCaCrl, trustedRoot);
    if (status != STATUS_OK)
    {
        return status;
    }

    const auto tcbSigningCert = chain.getTopmostCert();
    if(!_commonVerifier->checkSha256EcdsaSignature(
            enclaveIdentity.getSignature(), enclaveIdentity.getBody(), tcbSigningCert->getPubKey()))
    {
        LOG_ERROR("QE Identity signature verification failure.");
        return STATUS_SGX_ENCLAVE_IDENTITY_INVALID_SIGNATURE;
    }

    const auto validityDateTcbSigCert = tcbSigningCert->getValidity().getNotAfterTime();

    if(expirationDate > validityDateTcbSigCert)
    {
        LOG_ERROR("TCB Signing Certificate is expired. Expiration date: {}, validity: {}",
                  logger::timeToString(expirationDate), logger::timeToString(validityDateTcbSigCert));
        return STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED;
    }

    const auto rootCA = chain.getRootCert();
    const auto validityDateRootCA = rootCA->getValidity().getNotAfterTime();

    if(expirationDate > validityDateRootCA)
    {
        LOG_ERROR("TCB Signing Chain Root CA is expired. Expiration date: {}, validity: {}",
                  logger::timeToString(expirationDate), logger::timeToString(validityDateRootCA));
        return STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED;
    }

    if(rootCaCrl.expired(expirationDate))
    {
        LOG_ERROR("ROOT CA CRL is expired. Expiration date: {}, validity date range - from: {} to: {}",
                  logger::timeToString(expirationDate),
                  logger::timeToString(rootCaCrl.getValidity().notBeforeTime),
                  logger::timeToString(rootCaCrl.getValidity().notAfterTime));
        return STATUS_SGX_CRL_EXPIRED;
    }

    const auto enclaveIdentityNextUpdate = enclaveIdentity.getNextUpdate();

    if (expirationDate > enclaveIdentityNextUpdate)
    {
        LOG_ERROR("Enclave Identity is expired. Expiration date: {}, next update: {}",
                  logger::timeToString(expirationDate), logger::timeToString(enclaveIdentityNextUpdate));
        return STATUS_SGX_ENCLAVE_IDENTITY_EXPIRED;
    }

    return STATUS_OK;
}

}}} // namespace intel { namespace sgx { namespace dcap {
