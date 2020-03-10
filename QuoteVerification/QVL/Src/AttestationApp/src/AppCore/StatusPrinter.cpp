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

#include "StatusPrinter.h"
#include <array>
#include <string>

namespace {

std::string printStatus(Status s)
{
    static constexpr Status MAX_STATUS = STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED;
    static std::array<std::string, MAX_STATUS + 1> statusStrs = {{
        "STATUS_OK",
        "STATUS_UNSUPPORTED_CERT_FORMAT",

        "STATUS_SGX_ROOT_CA_MISSING",
        "STATUS_SGX_ROOT_CA_INVALID",
        "STATUS_SGX_ROOT_CA_INVALID_EXTENSIONS",
        "STATUS_SGX_ROOT_CA_INVALID_ISSUER",
        "STATUS_SGX_ROOT_CA_UNTRUSTED",

        "STATUS_SGX_INTERMEDIATE_CA_MISSING",
        "STATUS_SGX_INTERMEDIATE_CA_INVALID",
        "STATUS_SGX_INTERMEDIATE_CA_INVALID_EXTENSIONS",
        "STATUS_SGX_INTERMEDIATE_CA_INVALID_ISSUER",
        "STATUS_SGX_INTERMEDIATE_CA_REVOKED",

        "STATUS_SGX_PCK_MISSING",
        "STATUS_SGX_PCK_INVALID",
        "STATUS_SGX_PCK_INVALID_EXTENSIONS",
        "STATUS_SGX_PCK_INVALID_ISSUER",
        "STATUS_SGX_PCK_REVOKED",

        "STATUS_TRUSTED_ROOT_CA_INVALID",
        "STATUS_SGX_PCK_CERT_CHAIN_UNTRUSTED",

        "STATUS_SGX_TCB_INFO_UNSUPPORTED_FORMAT",
        "STATUS_SGX_TCB_INFO_INVALID",
        "STATUS_TCB_INFO_INVALID_SIGNATURE",

        "STATUS_SGX_TCB_SIGNING_CERT_MISSING",
        "STATUS_SGX_TCB_SIGNING_CERT_INVALID",
        "STATUS_SGX_TCB_SIGNING_CERT_INVALID_EXTENSIONS",
        "STATUS_SGX_TCB_SIGNING_CERT_INVALID_ISSUER",
        "STATUS_SGX_TCB_SIGNING_CERT_CHAIN_UNTRUSTED",
        "STATUS_SGX_TCB_SIGNING_CERT_REVOKED",

        "STATUS_SGX_CRL_UNSUPPORTED_FORMAT",
        "STATUS_SGX_CRL_UNKNOWN_ISSUER",
        "STATUS_SGX_CRL_INVALID",
        "STATUS_SGX_CRL_INVALID_EXTENSIONS",
        "STATUS_SGX_CRL_INVALID_SIGNATURE",


        "STATUS_SGX_CA_CERT_UNSUPPORTED_FORMAT",
        "STATUS_SGX_CA_CERT_INVALID",
        "STATUS_TRUSTED_ROOT_CA_UNSUPPORTED_FORMAT",

        "STATUS_MISSING_PARAMETERS",

        "STATUS_UNSUPPORTED_QUOTE_FORMAT",
        "STATUS_UNSUPPORTED_PCK_CERT_FORMAT",
        "STATUS_INVALID_PCK_CERT",
        "STATUS_UNSUPPORTED_PCK_RL_FORMAT",
        "STATUS_INVALID_PCK_CRL",
        "STATUS_UNSUPPORTED_TCB_INFO_FORMAT",
        "STATUS_PCK_REVOKED",
        "STATUS_TCB_INFO_MISMATCH",
        "STATUS_TCB_OUT_OF_DATE",
        "STATUS_TCB_REVOKED",
        "STATUS_TCB_CONFIGURATION_NEEDED",
        "STATUS_TCB_OUT_OF_DATE_CONFIGURATION_NEEDED",
        "STATUS_TCB_NOT_SUPPORTED",
        "STATUS_TCB_UNRECOGNIZED_STATUS",
        "STATUS_UNSUPPORTED_QE_CERTIFICATION",
        "STATUS_INVALID_QE_CERTIFICATION_DATA_SIZE",
        "STATUS_UNSUPPORTED_QE_CERTIFICATION_DATA_TYPE",
        "STATUS_PCK_CERT_MISMATCH",
        "STATUS_INVALID_QE_REPORT_SIGNATURE",
        "STATUS_INVALID_QE_REPORT_DATA",
        "STATUS_INVALID_QUOTE_SIGNATURE",

        "STATUS_SGX_QE_IDENTITY_UNSUPPORTED_FORMAT",
        "STATUS_SGX_QE_IDENTITY_INVALID",
        "STATUS_SGX_QE_IDENTITY_INVALID_SIGNATURE",

        "STATUS_SGX_ENCLAVE_REPORT_UNSUPPORTED_FORMAT",
        "STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_FORMAT",
        "STATUS_SGX_ENCLAVE_IDENTITY_INVALID",
        "STATUS_SGX_ENCLAVE_IDENTITY_UNSUPPORTED_VERSION",
        "STATUS_SGX_ENCLAVE_IDENTITY_OUT_OF_DATE",
        "STATUS_SGX_ENCLAVE_REPORT_MISCSELECT_MISMATCH",
        "STATUS_SGX_ENCLAVE_REPORT_ATTRIBUTES_MISMATCH",
        "STATUS_SGX_ENCLAVE_REPORT_MRENCLAVE_MISMATCH",
        "STATUS_SGX_ENCLAVE_REPORT_MRSIGNER_MISMATCH",
        "STATUS_SGX_ENCLAVE_REPORT_ISVPRODID_MISMATCH",
        "STATUS_SGX_ENCLAVE_REPORT_ISVSVN_OUT_OF_DATE",

        "STATUS_UNSUPPORTED_QE_IDENTITY_FORMAT",
        "STATUS_QE_IDENTITY_OUT_OF_DATE",
        "STATUS_QE_IDENTITY_MISMATCH",
        "STATUS_SGX_TCB_INFO_EXPIRED",
        "STATUS_SGX_ENCLAVE_IDENTITY_INVALID_SIGNATURE",
        "STATUS_INVALID_PARAMETER",
        "STATUS_SGX_PCK_CERT_CHAIN_EXPIRED",
        "STATUS_SGX_CRL_EXPIRED",
        "STATUS_SGX_SIGNING_CERT_CHAIN_EXPIRED",
        "STATUS_SGX_ENCLAVE_IDENTITY_EXPIRED",
        "STATUS_TCB_SW_HARDENING_NEEDED",
        "STATUS_TCB_CONFIGURATION_AND_SW_HARDENING_NEEDED",
        "STATUS_SGX_ENCLAVE_REPORT_ISVSVN_REVOKED"
    }};

    const auto statusNumberStr = "(" + std::to_string(s) + ")";
    if (s > MAX_STATUS)
    {
        return "Unknown status" + statusNumberStr;
    }
    return statusStrs[s] + statusNumberStr;
}
} // anonymous namespace

std::ostream& operator<<(std::ostream& os, Status status)
{
    os << printStatus(status);
    return os;
}

