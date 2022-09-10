/* server_wolfssl.c
 *
 * Copyright (C) 2006-2022 wolfSSL Inc.
 *
 * This file is part of wolfSSL.
 *
 * wolfSSL is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * wolfSSL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1335, USA
 */

#include  <Source/net_sock.h>
#include  <Source/net_app.h>
#include  <Source/net_util.h>
#include  <Source/net_ascii.h>
#include  <app_cfg.h>

#include  "wolfssl/ssl.h"
#include  "server_wolfssl.h"

#define TLS_SERVER_PORT 11111
#define TX_BUF_SIZE 64
#define RX_BUF_SIZE 1024
#define TCP_SERVER_CONN_Q_SIZE 1

/* derived from wolfSSL/certs/server-ecc.der */

static const CPU_INT08U server_ecc_der_256[] = { 0x30, 0x82, 0x03, 0x10,
        0x30, 0x82, 0x02, 0xB5, 0xA0, 0x03, 0x02, 0x01, 0x02, 0x02, 0x09, 0x00,
        0xEF, 0x46, 0xC7, 0xA4, 0x9B, 0xBB, 0x60, 0xD3, 0x30, 0x0A, 0x06, 0x08,
        0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03, 0x02, 0x30, 0x81, 0x8F, 0x31,
        0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53,
        0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x0A, 0x57,
        0x61, 0x73, 0x68, 0x69, 0x6E, 0x67, 0x74, 0x6F, 0x6E, 0x31, 0x10, 0x30,
        0x0E, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x07, 0x53, 0x65, 0x61, 0x74,
        0x74, 0x6C, 0x65, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A,
        0x0C, 0x07, 0x45, 0x6C, 0x69, 0x70, 0x74, 0x69, 0x63, 0x31, 0x0C, 0x30,
        0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x45, 0x43, 0x43, 0x31,
        0x18, 0x30, 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F, 0x77, 0x77,
        0x77, 0x2E, 0x77, 0x6F, 0x6C, 0x66, 0x73, 0x73, 0x6C, 0x2E, 0x63, 0x6F,
        0x6D, 0x31, 0x1F, 0x30, 0x1D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7,
        0x0D, 0x01, 0x09, 0x01, 0x16, 0x10, 0x69, 0x6E, 0x66, 0x6F, 0x40, 0x77,
        0x6F, 0x6C, 0x66, 0x73, 0x73, 0x6C, 0x2E, 0x63, 0x6F, 0x6D, 0x30, 0x1E,
        0x17, 0x0D, 0x31, 0x36, 0x30, 0x38, 0x31, 0x31, 0x32, 0x30, 0x30, 0x37,
        0x33, 0x38, 0x5A, 0x17, 0x0D, 0x31, 0x39, 0x30, 0x35, 0x30, 0x38, 0x32,
        0x30, 0x30, 0x37, 0x33, 0x38, 0x5A, 0x30, 0x81, 0x8F, 0x31, 0x0B, 0x30,
        0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x55, 0x53, 0x31, 0x13,
        0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x0A, 0x57, 0x61, 0x73,
        0x68, 0x69, 0x6E, 0x67, 0x74, 0x6F, 0x6E, 0x31, 0x10, 0x30, 0x0E, 0x06,
        0x03, 0x55, 0x04, 0x07, 0x0C, 0x07, 0x53, 0x65, 0x61, 0x74, 0x74, 0x6C,
        0x65, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07,
        0x45, 0x6C, 0x69, 0x70, 0x74, 0x69, 0x63, 0x31, 0x0C, 0x30, 0x0A, 0x06,
        0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x45, 0x43, 0x43, 0x31, 0x18, 0x30,
        0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F, 0x77, 0x77, 0x77, 0x2E,
        0x77, 0x6F, 0x6C, 0x66, 0x73, 0x73, 0x6C, 0x2E, 0x63, 0x6F, 0x6D, 0x31,
        0x1F, 0x30, 0x1D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01,
        0x09, 0x01, 0x16, 0x10, 0x69, 0x6E, 0x66, 0x6F, 0x40, 0x77, 0x6F, 0x6C,
        0x66, 0x73, 0x73, 0x6C, 0x2E, 0x63, 0x6F, 0x6D, 0x30, 0x59, 0x30, 0x13,
        0x06, 0x07, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x02, 0x01, 0x06, 0x08, 0x2A,
        0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0x03, 0x42, 0x00, 0x04, 0xBB,
        0x33, 0xAC, 0x4C, 0x27, 0x50, 0x4A, 0xC6, 0x4A, 0xA5, 0x04, 0xC3, 0x3C,
        0xDE, 0x9F, 0x36, 0xDB, 0x72, 0x2D, 0xCE, 0x94, 0xEA, 0x2B, 0xFA, 0xCB,
        0x20, 0x09, 0x39, 0x2C, 0x16, 0xE8, 0x61, 0x02, 0xE9, 0xAF, 0x4D, 0xD3,
        0x02, 0x93, 0x9A, 0x31, 0x5B, 0x97, 0x92, 0x21, 0x7F, 0xF0, 0xCF, 0x18,
        0xDA, 0x91, 0x11, 0x02, 0x34, 0x86, 0xE8, 0x20, 0x58, 0x33, 0x0B, 0x80,
        0x34, 0x89, 0xD8, 0xA3, 0x81, 0xF7, 0x30, 0x81, 0xF4, 0x30, 0x1D, 0x06,
        0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x5D, 0x5D, 0x26, 0xEF,
        0xAC, 0x7E, 0x36, 0xF9, 0x9B, 0x76, 0x15, 0x2B, 0x4A, 0x25, 0x02, 0x23,
        0xEF, 0xB2, 0x89, 0x30, 0x30, 0x81, 0xC4, 0x06, 0x03, 0x55, 0x1D, 0x23,
        0x04, 0x81, 0xBC, 0x30, 0x81, 0xB9, 0x80, 0x14, 0x5D, 0x5D, 0x26, 0xEF,
        0xAC, 0x7E, 0x36, 0xF9, 0x9B, 0x76, 0x15, 0x2B, 0x4A, 0x25, 0x02, 0x23,
        0xEF, 0xB2, 0x89, 0x30, 0xA1, 0x81, 0x95, 0xA4, 0x81, 0x92, 0x30, 0x81,
        0x8F, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02,
        0x55, 0x53, 0x31, 0x13, 0x30, 0x11, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C,
        0x0A, 0x57, 0x61, 0x73, 0x68, 0x69, 0x6E, 0x67, 0x74, 0x6F, 0x6E, 0x31,
        0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x07, 0x53, 0x65,
        0x61, 0x74, 0x74, 0x6C, 0x65, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55,
        0x04, 0x0A, 0x0C, 0x07, 0x45, 0x6C, 0x69, 0x70, 0x74, 0x69, 0x63, 0x31,
        0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C, 0x03, 0x45, 0x43,
        0x43, 0x31, 0x18, 0x30, 0x16, 0x06, 0x03, 0x55, 0x04, 0x03, 0x0C, 0x0F,
        0x77, 0x77, 0x77, 0x2E, 0x77, 0x6F, 0x6C, 0x66, 0x73, 0x73, 0x6C, 0x2E,
        0x63, 0x6F, 0x6D, 0x31, 0x1F, 0x30, 0x1D, 0x06, 0x09, 0x2A, 0x86, 0x48,
        0x86, 0xF7, 0x0D, 0x01, 0x09, 0x01, 0x16, 0x10, 0x69, 0x6E, 0x66, 0x6F,
        0x40, 0x77, 0x6F, 0x6C, 0x66, 0x73, 0x73, 0x6C, 0x2E, 0x63, 0x6F, 0x6D,
        0x82, 0x09, 0x00, 0xEF, 0x46, 0xC7, 0xA4, 0x9B, 0xBB, 0x60, 0xD3, 0x30,
        0x0C, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x04, 0x05, 0x30, 0x03, 0x01, 0x01,
        0xFF, 0x30, 0x0A, 0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x04, 0x03,
        0x02, 0x03, 0x49, 0x00, 0x30, 0x46, 0x02, 0x21, 0x00, 0xF1, 0xD0, 0xA6,
        0x3E, 0x83, 0x33, 0x24, 0xD1, 0x7A, 0x05, 0x5F, 0x1E, 0x0E, 0xBD, 0x7D,
        0x6B, 0x33, 0xE9, 0xF2, 0x86, 0xF3, 0xF3, 0x3D, 0xA9, 0xEF, 0x6A, 0x87,
        0x31, 0xB3, 0xB7, 0x7E, 0x50, 0x02, 0x21, 0x00, 0xF0, 0x60, 0xDD, 0xCE,
        0xA2, 0xDB, 0x56, 0xEC, 0xD9, 0xF4, 0xE4, 0xE3, 0x25, 0xD4, 0xB0, 0xC9,
        0x25, 0x7D, 0xCA, 0x7A, 0x5D, 0xBA, 0xC4, 0xB2, 0xF6, 0x7D, 0x04, 0xC7,
        0xBD, 0x62, 0xC9, 0x20 };

/* derived from wolfSSL/certs/ecc-key.der */

static const CPU_INT08U ecc_key_der_256[] = { 0x30, 0x77, 0x02, 0x01, 0x01,
        0x04, 0x20, 0x45, 0xB6, 0x69, 0x02, 0x73, 0x9C, 0x6C, 0x85, 0xA1, 0x38,
        0x5B, 0x72, 0xE8, 0xE8, 0xC7, 0xAC, 0xC4, 0x03, 0x8D, 0x53, 0x35, 0x04,
        0xFA, 0x6C, 0x28, 0xDC, 0x34, 0x8D, 0xE1, 0xA8, 0x09, 0x8C, 0xA0, 0x0A,
        0x06, 0x08, 0x2A, 0x86, 0x48, 0xCE, 0x3D, 0x03, 0x01, 0x07, 0xA1, 0x44,
        0x03, 0x42, 0x00, 0x04, 0xBB, 0x33, 0xAC, 0x4C, 0x27, 0x50, 0x4A, 0xC6,
        0x4A, 0xA5, 0x04, 0xC3, 0x3C, 0xDE, 0x9F, 0x36, 0xDB, 0x72, 0x2D, 0xCE,
        0x94, 0xEA, 0x2B, 0xFA, 0xCB, 0x20, 0x09, 0x39, 0x2C, 0x16, 0xE8, 0x61,
        0x02, 0xE9, 0xAF, 0x4D, 0xD3, 0x02, 0x93, 0x9A, 0x31, 0x5B, 0x97, 0x92,
        0x21, 0x7F, 0xF0, 0xCF, 0x18, 0xDA, 0x91, 0x11, 0x02, 0x34, 0x86, 0xE8,
        0x20, 0x58, 0x33, 0x0B, 0x80, 0x34, 0x89, 0xD8 };


int wolfssl_server_test(void)
{
    NET_ERR err;
    NET_SOCK_ID sock_listen;
    NET_SOCK_ID sock_req;
    NET_SOCK_ADDR_IPv4 server_addr;
    NET_SOCK_ADDR_LEN server_addr_len;
    NET_SOCK_ADDR_IPv4 client_sock_addr_ip;
    NET_SOCK_ADDR_LEN client_sock_addr_ip_size;
    CPU_CHAR rx_buf[RX_BUF_SIZE];
    CPU_CHAR tx_buf[TX_BUF_SIZE];
    CPU_BOOLEAN attempt_conn;
    OS_ERR os_err;
    WOLFSSL * ssl;
    WOLFSSL_CTX * ctx;
    int tx_buf_sz = 0, ret = 0, error = 0;

    #ifdef DEBUG_WOLFSSL
        wolfSSL_Debugging_ON();
    #endif

    /* wolfSSL INIT and CTX SETUP */

    wolfSSL_Init();

    /* SET UP NETWORK SOCKET */

    APP_TRACE_INFO(("Opening network socket...\r\n"));
    sock_listen = NetSock_Open(NET_SOCK_ADDR_FAMILY_IP_V4,
                               NET_SOCK_TYPE_STREAM,
                               NET_SOCK_PROTOCOL_TCP,
                               &err);
    if (err != NET_SOCK_ERR_NONE) {
        APP_TRACE_INFO(("ERROR: NetSock_Open, err = %d\r\n", (int) err));
        return -1;
    }

    APP_TRACE_INFO(("Clearing memory for server_addr struct\r\n"));
    server_addr_len = sizeof(server_addr);
    Mem_Clr((void *) &server_addr, (CPU_SIZE_T) server_addr_len);

    APP_TRACE_INFO(("Setting up server_addr struct\r\n"));
    server_addr.AddrFamily = NET_SOCK_ADDR_FAMILY_IP_V4;
    server_addr.Addr = NET_UTIL_HOST_TO_NET_32(NET_SOCK_ADDR_IP_V4_WILDCARD);
    server_addr.Port = NET_UTIL_HOST_TO_NET_16(TLS_SERVER_PORT);

    NetSock_Bind((NET_SOCK_ID) sock_listen,
                (NET_SOCK_ADDR*) &server_addr,
                (NET_SOCK_ADDR_LEN) NET_SOCK_ADDR_SIZE,
                (NET_ERR*) &err);
    if (err != NET_SOCK_ERR_NONE) {
       APP_TRACE_INFO(("ERROR: NetSock_Bind, err = %d\r\n", (int) err));
       NetSock_Close(sock_listen, &err);
       return -1;
    }

    ctx = wolfSSL_CTX_new(wolfTLSv1_2_server_method());
    if (ctx == 0) {
        APP_TRACE_INFO(("ERROR: wolfSSL_CTX_new failed\r\n"));
        NetSock_Close(sock_listen, &err);
        return -1;
    }
    APP_TRACE_INFO(("wolfSSL_CTX_new done\r\n"));

    ret = wolfSSL_CTX_use_certificate_buffer(ctx,
                                             server_ecc_der_256,
                                             sizeof(server_ecc_der_256),
                                             SSL_FILETYPE_ASN1);
    if (ret != SSL_SUCCESS) {
        APP_TRACE_INFO(
                ("ERROR: wolfSSL_CTX_use_certificate_buffer() failed\r\n"));
        NetSock_Close(sock_listen, &err);
        wolfSSL_CTX_free(ctx);
        return -1;
    }
    ret = wolfSSL_CTX_use_PrivateKey_buffer(ctx,
                                            ecc_key_der_256,
                                            sizeof(ecc_key_der_256),
                                            SSL_FILETYPE_ASN1);
    if (ret != SSL_SUCCESS) {
        APP_TRACE_INFO(
                ("ERROR: wolfSSL_CTX_use_PrivateKey_buffer() failed\r\n"));
        NetSock_Close(sock_listen, &err);
        wolfSSL_CTX_free(ctx);
        return -1;
    }
    /* accept client socket connections */

    APP_TRACE_INFO(("Listening for client connection\r\n"));

    NetSock_Listen(sock_listen, TCP_SERVER_CONN_Q_SIZE, &err);
    if (err != NET_SOCK_ERR_NONE) {
        APP_TRACE_INFO(("ERROR: NetSock_Listen, err = %d\r\n", (int) err));
        NetSock_Close(sock_listen, &err);
        wolfSSL_CTX_free(ctx);
        return -1;
    }
    do {
        client_sock_addr_ip_size = sizeof(client_sock_addr_ip);
        sock_req = NetSock_Accept((NET_SOCK_ID) sock_listen,
                                 (NET_SOCK_ADDR*) &client_sock_addr_ip,
                                 (NET_SOCK_ADDR_LEN*) &client_sock_addr_ip_size,
                                 (NET_ERR*) &err);
        switch (err) {
        case NET_SOCK_ERR_NONE:
            attempt_conn = DEF_NO;
            break;
        case NET_ERR_INIT_INCOMPLETE:
        case NET_SOCK_ERR_NULL_PTR:
        case NET_SOCK_ERR_NONE_AVAIL:
        case NET_SOCK_ERR_CONN_ACCEPT_Q_NONE_AVAIL:
            attempt_conn = DEF_YES;
            break;
        case NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT:
            APP_TRACE_INFO(
                    ("NetSockAccept err = NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT\r\n"));
            attempt_conn = DEF_YES;
            break;
        default:
            attempt_conn = DEF_NO;
            break;
        }
    } while (attempt_conn == DEF_YES);
    if (err != NET_SOCK_ERR_NONE) {
        APP_TRACE_INFO(("ERROR: NetSock_Accept, err = %d\r\n", (int) err));
        NetSock_Close(sock_listen, &err);
        return -1;
    }

    APP_TRACE_INFO(("Got client connection! Starting TLS negotiation\r\n"));
    /* set up wolfSSL session */
    if ((ssl = wolfSSL_new(ctx)) == NULL) {
        APP_TRACE_INFO(("ERROR: wolfSSL_new() failed\r\n"));
        NetSock_Close(sock_req, &err);
        NetSock_Close(sock_listen, &err);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    APP_TRACE_INFO(("wolfSSL_new done\r\n"));
    ret = wolfSSL_set_fd(ssl, sock_req);
    if (ret != SSL_SUCCESS) {
        APP_TRACE_INFO(("ERROR: wolfSSL_set_fd() failed\r\n"));
        NetSock_Close(sock_req, &err);
        NetSock_Close(sock_listen, &err);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        return -1;
    }

    APP_TRACE_INFO(("wolfSSL_set_fd done\r\n"));
    do {
        error = 0; /* reset error */
        if (ret != SSL_SUCCESS) {
            error = wolfSSL_get_error(ssl, 0);
            APP_TRACE_INFO(
                    ("ERROR: wolfSSL_accept() failed, err = %d\r\n", error));
            if (error != SSL_ERROR_WANT_READ) {
                NetSock_Close(sock_req, &err);
                NetSock_Close(sock_listen, &err);
                wolfSSL_free(ssl);
                wolfSSL_CTX_free(ctx);
                return -1;
            }
            OSTimeDlyHMSM(0u, 0u, 0u, 500u, OS_OPT_TIME_HMSM_STRICT, &os_err);
        }
    } while ((ret != SSL_SUCCESS) && (error == SSL_ERROR_WANT_READ));

    APP_TRACE_INFO(("wolfSSL_accept() ok...\r\n"));

    /* read client data */

    error = 0;
    Mem_Set(rx_buf, 0, RX_BUF_SIZE);
    ret = wolfSSL_read(ssl, rx_buf, RX_BUF_SIZE - 1);
    if (ret < 0) {
        error = wolfSSL_get_error(ssl, 0);
        if (error != SSL_ERROR_WANT_READ) {
            APP_TRACE_INFO(("wolfSSL_read failed, error = %d\r\n", error));
            NetSock_Close(sock_req, &err);
            NetSock_Close(sock_listen, &err);
            wolfSSL_free(ssl);
            wolfSSL_CTX_free(ctx);
            return -1;
        }
    }

    APP_TRACE_INFO(("AFTER wolfSSL_read() call, ret = %d\r\n", ret));
    if (ret > 0) {
        rx_buf[ret] = 0;
        APP_TRACE_INFO(("Client sent: %s\r\n", rx_buf));
    }
    /* write response to client */
    Mem_Set(tx_buf, 0, TX_BUF_SIZE);
    tx_buf_sz = 22;
    Str_Copy_N(tx_buf, "I hear ya fa shizzle!\n", tx_buf_sz);
    if (wolfSSL_write(ssl, tx_buf, tx_buf_sz) != tx_buf_sz) {
        error = wolfSSL_get_error(ssl, 0);
        APP_TRACE_INFO(("ERROR: wolfSSL_write() failed, err = %d\r\n", error));
        NetSock_Close(sock_req, &err);
        NetSock_Close(sock_listen, &err);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        return -1;
    }
    ret = wolfSSL_shutdown(ssl);
    if (ret == SSL_SHUTDOWN_NOT_DONE)
        wolfSSL_shutdown(ssl);
        wolfSSL_free(ssl);
        wolfSSL_CTX_free(ctx);
        wolfSSL_Cleanup();
        NetSock_Close(sock_req, &err);
        NetSock_Close(sock_listen, &err);
    return 0;
}
