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

#include "PckCertVerifier.h"
#include "Utils/Logger.h"
#include "Utils/TimeUtils.h"

#include <OpensslHelpers/SignatureVerification.h>
#include <CertVerification/X509Constants.h>
#include <OpensslHelpers/KeyUtils.h>

namespace intel { namespace sgx { namespace dcap {

PckCertVerifier::PckCertVerifier() : _commonVerifier(new CommonVerifier()),
                                     _crlVerifier(new PckCrlVerifier())
{
}

PckCertVerifier::PckCertVerifier(std::unique_ptr<CommonVerifier>&& commonVerifier,
                                 std::unique_ptr<PckCrlVerifier>&& crlVerifier)
                                 : _commonVerifier(std::move(commonVerifier)), _crlVerifier(std::move(crlVerifier))
{
}

Status PckCertVerifier::verify(const CertificateChain &chain,
                               const pckparser::CrlStore &rootCaCrl,
                               const pckparser::CrlStore &intermediateCrl,
                               const dcap::parser::x509::Certificate &rootCa,
                               const std::time_t& expirationDate) const
{
    const auto x509InChainRootCa = chain.getRootCert();
    if(!x509InChainRootCa)
    {
        LOG_ERROR("ROOT CA is missing");
        return STATUS_SGX_ROOT_CA_MISSING;
    }

    if(!_baseVerifier.commonNameContains(x509InChainRootCa->getSubject(), constants::SGX_ROOT_CA_CN_PHRASE))
    {
        LOG_ERROR("RootCa from chain. CN in Subject field does not contain \"SGX Root CA\" phrase");
        return STATUS_SGX_ROOT_CA_MISSING;
    }

    const auto x509InChainIntermediateCa = chain.getIntermediateCert();
    if(!x509InChainIntermediateCa)
    {
        LOG_ERROR("Intermediate CA is missing");
        return STATUS_SGX_INTERMEDIATE_CA_MISSING;
    }

    if(!_baseVerifier.commonNameContains(x509InChainIntermediateCa->getSubject(), constants::SGX_INTERMEDIATE_CN_PHRASE))
    {
        LOG_ERROR("IntermediateCa from chain. CN in Subject field does not contain \"CA\" phrase");
        return STATUS_SGX_INTERMEDIATE_CA_MISSING;
    }

    const auto x509InChainPckCert = chain.getPckCert();
    if(!x509InChainPckCert)
    {
        LOG_ERROR("PCK cert is missing");
        return STATUS_SGX_PCK_MISSING;
    }

    if(!_baseVerifier.commonNameContains(x509InChainPckCert->getSubject(), constants::SGX_PCK_CN_PHRASE))
    {
        LOG_ERROR("PCK Cert from chain. CN in Subject field does not contain \"SGX PCK Certificate\" phrase");
        return STATUS_SGX_PCK_MISSING;
    }

    const auto rootVerificationStatus = _commonVerifier->verifyRootCACert(*x509InChainRootCa);
    if(rootVerificationStatus != STATUS_OK)
    {
        LOG_ERROR("Root CA verification failed: {}", rootVerificationStatus);
        return rootVerificationStatus;
    }

    const auto intermediateVerificationStatus = _commonVerifier->verifyIntermediate(*x509InChainIntermediateCa, *x509InChainRootCa);
    if(intermediateVerificationStatus != STATUS_OK)
    {
        LOG_ERROR("Intermediate CA verification failed: {}", intermediateVerificationStatus);
        return intermediateVerificationStatus;
    }

    const auto pckVerificationStatus = verifyPCKCert(*x509InChainPckCert, *x509InChainIntermediateCa);
    if(pckVerificationStatus != STATUS_OK)
    {
        LOG_ERROR("PCK Certificate verification failed: {}", pckVerificationStatus);
        return pckVerificationStatus;
    } 

    if(rootCa.getSubject() != rootCa.getIssuer())
    {
        LOG_ERROR("PCK RootCA is not self signed");
        return STATUS_TRUSTED_ROOT_CA_INVALID;
    }

    if(x509InChainRootCa->getSignature().getRawDer() != rootCa.getSignature().getRawDer())
    {
        LOG_ERROR("Signature of trusted root doesn't match signature of root cert from PCK Cert Chain. Chain is not trusted.");
        return STATUS_SGX_PCK_CERT_CHAIN_UNTRUSTED;
    }

    // 
    // begin of CRL verification
    //
    const auto checkRootCaCrlCorrectness = _crlVerifier->verify(rootCaCrl, *x509InChainRootCa);
    if(checkRootCaCrlCorrectness != STATUS_OK)
    {
        LOG_ERROR("PCK Revocation lists - RootCaCrl verification failed: {}", checkRootCaCrlCorrectness);
        return checkRootCaCrlCorrectness;
    } 

    const auto checkIntermediateCrlCorrectness = _crlVerifier->verify(intermediateCrl, *x509InChainIntermediateCa);
    if(checkIntermediateCrlCorrectness != STATUS_OK)
    {
        LOG_ERROR("PCK Revocation lists - IntermediateCaCrl verification failed: {}", checkIntermediateCrlCorrectness);
        return checkIntermediateCrlCorrectness;
    }

    if(rootCaCrl.isRevoked(*x509InChainIntermediateCa))
    {
        LOG_ERROR("Intermediate CA Cert is revoked by Root CA");
        return STATUS_SGX_INTERMEDIATE_CA_REVOKED;
    }

    if(intermediateCrl.isRevoked(*x509InChainPckCert))
    {
        LOG_ERROR("PCK Cert is revoked by Intermediate CA");
        return STATUS_SGX_PCK_REVOKED;
    }

    const auto validityDateRootCA = x509InChainRootCa->getValidity().getNotAfterTime();
    if(expirationDate > validityDateRootCA)
    {
        LOG_ERROR("PCK Cert Chain Root CA is expired. Expiration date: {}, validity: {}",
                  logger::timeToString(expirationDate), logger::timeToString(validityDateRootCA));
        return STATUS_SGX_PCK_CERT_CHAIN_EXPIRED;
    }

    auto const validityDateIntermediateCA = x509InChainIntermediateCa->getValidity().getNotAfterTime();
    if(expirationDate > validityDateIntermediateCA)
    {
        LOG_ERROR("PCK Cert Chain Intermediate CA is expired. Expiration date: {}, validity: {}",
                  logger::timeToString(expirationDate), logger::timeToString(validityDateIntermediateCA));
        return STATUS_SGX_PCK_CERT_CHAIN_EXPIRED;
    }

    auto const validityDatePCK = x509InChainPckCert->getValidity().getNotAfterTime();
    if(expirationDate > validityDatePCK)
    {
        LOG_ERROR("PCK Cert Chain PCK Cert is expired. Expiration date: {}, validity: {}",
                  logger::timeToString(expirationDate), logger::timeToString(validityDatePCK));
        return STATUS_SGX_PCK_CERT_CHAIN_EXPIRED;
    }

    if(rootCaCrl.expired(expirationDate))
    {
        LOG_ERROR("ROOT CA CRL is expired. Expiration date: {}, validity date range - from: {} to: {}",
                  logger::timeToString(expirationDate),
                  logger::timeToString(rootCaCrl.getValidity().notBeforeTime),
                  logger::timeToString(rootCaCrl.getValidity().notAfterTime));
        return STATUS_SGX_CRL_EXPIRED;
    }

    if(intermediateCrl.expired(expirationDate))
    {
        LOG_ERROR("Intermediate CA CRL is expired. Expiration date: {}, validity date range - from: {} to: {}",
                  logger::timeToString(expirationDate),
                  logger::timeToString(rootCaCrl.getValidity().notBeforeTime),
                  logger::timeToString(rootCaCrl.getValidity().notAfterTime));
        return STATUS_SGX_CRL_EXPIRED;
    }

    return STATUS_OK;
}

Status PckCertVerifier::verifyPCKCert(const dcap::parser::x509::PckCertificate &pckCert,
                                      const dcap::parser::x509::Certificate &intermediate) const
{
    if(pckCert.getIssuer() != intermediate.getSubject())
    {
        LOG_ERROR("PCK Cert is not signed by Intermediate CA");
        return STATUS_SGX_PCK_INVALID_ISSUER;
    }

    if(!_commonVerifier->checkSignature(pckCert, intermediate))
    {
        LOG_ERROR("PCK Cert signature is invalid");
        return STATUS_SGX_PCK_INVALID_ISSUER;
    }

    return STATUS_OK;
}

}}} //namespace intel { namespace sgx { namespace dcap
