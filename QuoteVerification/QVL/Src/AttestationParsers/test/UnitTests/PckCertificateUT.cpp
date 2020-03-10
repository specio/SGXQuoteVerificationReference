/*
 * Copyright (C) 2011-2020 Intel Corporation. All rights reserved.
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

#include <gtest/gtest.h>
#include <gmock/gmock-generated-matchers.h>

#include "SgxEcdsaAttestation/AttestationParsers.h"

#include "X509TestConstants.h"
#include "X509CertGenerator.h"

using namespace intel::sgx::dcap;
using namespace intel::sgx::dcap::parser;
using namespace ::testing;


struct PckCertificateUT: public testing::Test {
    int timeNow = 0;
    int timeOneHour = 3600;

    Bytes sn { 0x40, 0x66, 0xB0, 0x01, 0x4B, 0x71, 0x7C, 0xF7, 0x01, 0xD5,
               0xB7, 0xD8, 0xF1, 0x36, 0xB1, 0x99, 0xE9, 0x73, 0x96, 0xC8 };
    Bytes ppid = Bytes(16, 0xaa);
    Bytes cpusvn = Bytes(16, 0x09);
    Bytes pcesvn = {0x03, 0xf2};
    Bytes pceId = {0x04, 0xf3};
    Bytes fmspc = {0x05, 0xf4, 0x44, 0x45, 0xaa, 0x00};
    test::X509CertGenerator certGenerator;

    crypto::EVP_PKEY_uptr keyRoot = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr keyInt = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::EVP_PKEY_uptr key = crypto::make_unique<EVP_PKEY>(nullptr);
    crypto::X509_uptr rootCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr intCert = crypto::make_unique<X509>(nullptr);
    crypto::X509_uptr cert = crypto::make_unique<X509>(nullptr);

    std::string pemPckCert, pemIntCert, pemRootCert;

    PckCertificateUT()
    {
        keyRoot = certGenerator.generateEcKeypair();
        keyInt = certGenerator.generateEcKeypair();
        key = certGenerator.generateEcKeypair();
        rootCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, keyRoot.get(), keyRoot.get(),
                                                constants::ROOT_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        intCert = certGenerator.generateCaCert(2, sn, timeNow, timeOneHour, keyInt.get(), keyRoot.get(),
                                               constants::PLATFORM_CA_SUBJECT, constants::ROOT_CA_SUBJECT);

        cert = certGenerator.generatePCKCert(2, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                             constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                             ppid, cpusvn, pcesvn, pceId, fmspc);

        pemPckCert = certGenerator.x509ToString(cert.get());
        pemIntCert = certGenerator.x509ToString(intCert.get());
        pemRootCert = certGenerator.x509ToString(rootCert.get());
    }
};

TEST_F(PckCertificateUT, pckCertificateParse)
{
    ASSERT_NO_THROW(x509::PckCertificate::parse(pemPckCert));
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::PckCertificate::parse(pemIntCert), InvalidExtensionException);
    // Exception thrown because of missing SGX TCB extensions
    ASSERT_THROW(x509::PckCertificate::parse(pemRootCert), InvalidExtensionException);
}

TEST_F(PckCertificateUT, pckCertificateConstructors)
{
    const auto& certificate = x509::Certificate::parse(pemPckCert);
    const auto& pckCertificateFromCert = x509::PckCertificate(certificate);
    const auto& pckCertificate = x509::PckCertificate::parse(pemPckCert);

    ASSERT_EQ(pckCertificateFromCert.getVersion(), pckCertificate.getVersion());
    ASSERT_EQ(pckCertificateFromCert.getSerialNumber(), pckCertificate.getSerialNumber());
    ASSERT_EQ(pckCertificateFromCert.getSubject(), pckCertificate.getSubject());
    ASSERT_EQ(pckCertificateFromCert.getIssuer(), pckCertificate.getIssuer());
    ASSERT_EQ(pckCertificateFromCert.getValidity(), pckCertificate.getValidity());
    ASSERT_EQ(pckCertificateFromCert.getExtensions(), pckCertificate.getExtensions());
    ASSERT_EQ(pckCertificateFromCert.getSignature(), pckCertificate.getSignature());
    ASSERT_EQ(pckCertificateFromCert.getPubKey(), pckCertificate.getPubKey());

    ASSERT_EQ(pckCertificateFromCert.getTcb(), pckCertificate.getTcb());
    ASSERT_EQ(pckCertificateFromCert.getPpid(), pckCertificate.getPpid());
    ASSERT_EQ(pckCertificateFromCert.getPceId(), pckCertificate.getPceId());
    ASSERT_EQ(pckCertificateFromCert.getSgxType(), pckCertificate.getSgxType());
}

TEST_F(PckCertificateUT, pckCertificateGetters)
{
    const auto& pckCertificate = x509::PckCertificate::parse(pemPckCert);

    ASSERT_EQ(pckCertificate.getVersion(), 3);
    ASSERT_THAT(pckCertificate.getSerialNumber(), ElementsAreArray(sn));

    auto ecKey = crypto::make_unique(EVP_PKEY_get1_EC_KEY(key.get()));
    uint8_t *pubKey = nullptr;
    auto pKeyLen = EC_KEY_key2buf(ecKey.get(), EC_KEY_get_conv_form(ecKey.get()), &pubKey, NULL);
    std::vector<uint8_t> expectedPublicKey { pubKey, pubKey + pKeyLen };

    ASSERT_THAT(pckCertificate.getPubKey(), ElementsAreArray(expectedPublicKey));
    ASSERT_EQ(pckCertificate.getIssuer(), constants::PLATFORM_CA_SUBJECT);
    ASSERT_EQ(pckCertificate.getSubject(), constants::PCK_SUBJECT);
    ASSERT_NE(pckCertificate.getIssuer(), pckCertificate.getSubject()); // PCK certificate should not be self-signed

    ASSERT_LT(pckCertificate.getValidity().getNotBeforeTime(), pckCertificate.getValidity().getNotAfterTime());

    const std::vector<x509::Extension> expectedExtensions = constants::PCK_X509_EXTENSIONS;
    ASSERT_THAT(pckCertificate.getExtensions().size(), expectedExtensions.size());

    ASSERT_THAT(pckCertificate.getPpid(), ElementsAreArray(ppid));
    ASSERT_THAT(pckCertificate.getPceId(), ElementsAreArray(pceId));
    ASSERT_THAT(pckCertificate.getFmspc(), ElementsAreArray(fmspc));
    ASSERT_EQ(pckCertificate.getSgxType(), x509::SgxType::Standard);

    free(pubKey);
}

TEST_F(PckCertificateUT, certificateOperators)
{
    const auto& certificate1 = x509::PckCertificate::parse(pemPckCert);
    const auto& certificate2 = x509::PckCertificate::parse(pemPckCert);
    const auto ucert = certGenerator.generatePCKCert(3, sn, timeNow, timeOneHour, key.get(), keyInt.get(),
                                                     constants::PCK_SUBJECT, constants::PLATFORM_CA_SUBJECT,
                                                     ppid, cpusvn, pcesvn, pceId, fmspc);
    const auto pemCert = certGenerator.x509ToString(ucert.get());
    const auto& certificate3 = x509::PckCertificate::parse(pemCert);

    ASSERT_EQ(certificate1, certificate2);
    ASSERT_FALSE(certificate1 == certificate3);
    ASSERT_FALSE(certificate2 == certificate3);
}
