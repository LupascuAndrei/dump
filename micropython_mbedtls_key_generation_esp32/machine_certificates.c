#include "modmachine.h"

#include "mbedtls/config.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/certs.h"
#include "mbedtls/x509.h"
#include "mbedtls/ssl.h"
//#include "mbedtls/ssl_cookie.h"
//#include "mbedtls/net_sockets.h"
#include "mbedtls/error.h"
#include "mbedtls/debug.h"
#include "mbedtls/timing.h"
#include "mbedtls/x509_csr.h"

#include "py/runtime0.h"
#include "py/nlr.h"
#include "py/objlist.h"
#include "py/objstr.h"


#include "mbedtls/pk.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/rsa.h"

// KEY GENERATION
static mbedtls_pk_context global_key; 
/* workaround for not including additional headers in module_ssl.c .
we free the memory used by this when finishing creating the certificate */
static void generate_key(unsigned char* output_buf);
static int write_private_key(unsigned char* output_buf);
static int ssl_context_create_private_key(unsigned char* output_buf)
{
	generate_key(output_buf);
	return write_private_key(output_buf );
}

#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif

#if defined(MBEDTLS_PLATFORM_C)
#include "mbedtls/platform.h"
#else
#include <stdio.h>
#define mbedtls_printf     printf
#endif

#if defined(MBEDTLS_PK_WRITE_C) && \
    defined(MBEDTLS_ENTROPY_C) && defined(MBEDTLS_CTR_DRBG_C)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DEV_RANDOM_THRESHOLD        32
#endif

#if defined(MBEDTLS_ECP_C)
#define DFL_EC_CURVE            mbedtls_ecp_curve_list()->grp_id
#else
#define DFL_EC_CURVE            0
#endif

#if !defined(_WIN32) && defined(MBEDTLS_FS_IO)
#define USAGE_DEV_RANDOM \
     "    use_dev_random=0|1    default: 0\n"
#else
#define USAGE_DEV_RANDOM ""
#endif 

#define FORMAT_PEM              0
#define FORMAT_DER              1

#define DFL_TYPE                MBEDTLS_PK_RSA
#define DFL_RSA_KEYSIZE         1024
#define DFL_FORMAT              FORMAT_PEM
#define DFL_USE_DEV_RANDOM      0
 
/*
type=rsa|ec           default: rsa\n"          
rsa_keysize=%%d        default: 1024\n"        
ec_curve=%%s           see below\n"            
format=pem|der        default: pem\n"          
USAGE_DEV_RANDOM                                    

*/
struct options_key
{
    int type;                   /* the type of key to generate          */
    int rsa_keysize;            /* length of key in bits                */
    int ec_curve;               /* curve identifier for EC keys         */
    int format;                 /* the output format to use             */
    int use_dev_random;         /* use /dev/random as entropy source    */
} opt_key;
 
static int write_private_key( unsigned char* output_buf)
{
    int ret = 0;
    if( opt_key.format == FORMAT_PEM )
    {
        if( ( ret = mbedtls_pk_write_key_pem(&global_key, output_buf, 1024 ) ) != 0 )
            return( ret );
        ret = strlen( (char *) output_buf );
    }
    //todo : support FORMAT_DER
    return ret;

}
 
static void generate_key(unsigned char* output_buf)
{
    int ret = 1;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "gen_key";
    mbedtls_pk_init( &global_key );
    mbedtls_ctr_drbg_init( &ctr_drbg );
    opt_key.type                = DFL_TYPE;
    opt_key.rsa_keysize         = DFL_RSA_KEYSIZE;
    opt_key.ec_curve            = DFL_EC_CURVE;
    opt_key.format              = DFL_FORMAT;
    opt_key.use_dev_random      = DFL_USE_DEV_RANDOM;
    mbedtls_entropy_init( &entropy );
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                                                        (const unsigned char *) pers,
                                                        strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( "Certificate creation failed\n  ! mbedtls_ctr_drbg_seed returned -0x%04x\n", -ret );
        goto exit;
	}


    if( ( ret = mbedtls_pk_setup( &global_key, mbedtls_pk_info_from_type( opt_key.type ) ) ) != 0 )
    {
        mbedtls_printf( "Certificate creation  failed\n  !  mbedtls_pk_setup returned -0x%04x", -ret );
        goto exit;
    }

#if defined(MBEDTLS_RSA_C) && defined(MBEDTLS_GENPRIME)
    if( opt_key.type == MBEDTLS_PK_RSA )
    {
        ret = mbedtls_rsa_gen_key( mbedtls_pk_rsa( global_key ), mbedtls_ctr_drbg_random, &ctr_drbg,
                                                        opt_key.rsa_keysize, 65537 );
        if( ret != 0 )
        {
            mbedtls_printf( "Certificate creation  failed\n  !  mbedtls_rsa_gen_key returned -0x%04x", -ret );
            goto exit;
        }
	}
	else
#endif /* MBEDTLS_RSA_C */
#if defined(MBEDTLS_ECP_C)
    if( opt_key.type == MBEDTLS_PK_ECKEY )
    {
        ret = mbedtls_ecp_gen_key( opt_key.ec_curve, mbedtls_pk_ec( global_key ),
                                            mbedtls_ctr_drbg_random, &ctr_drbg );
        if( ret != 0 )
        {
                mbedtls_printf( "Certificate creation  failed\n  !  mbedtls_rsa_gen_key returned -0x%04x", -ret );
                goto exit;
        }
    }
    else
#endif /* MBEDTLS_ECP_C */
    {
        mbedtls_printf( "Certificate creation  failed\n  !  global_key type not supported\n" );
        goto exit;
    }
exit:
    mbedtls_entropy_free( &entropy );
    mbedtls_ctr_drbg_free( &ctr_drbg );
    return;
}


// CERTIFICATE GENERATION
#if defined(MBEDTLS_X509_CSR_PARSE_C)
#define USAGE_CSR                                                           \
    "    request_file=%%s     default: (empty)\n"                           \
    "                        If request_file is specified, subject_key,\n"  \
    "                        subject_pwd and subject_name are ignored!\n"
#else
#define USAGE_CSR ""
#endif /* MBEDTLS_X509_CSR_PARSE_C */

#define DFL_ISSUER_CRT          ""
#define DFL_REQUEST_FILE        ""
#define DFL_ISSUER_KEY          "ca.key"
#define DFL_SUBJECT_PWD         ""
#define DFL_ISSUER_PWD          ""
#define DFL_SUBJECT_NAME        "CN=Cert,O=mbed TLS,C=UK"
#define DFL_ISSUER_NAME         "CN=CA,O=mbed TLS,C=UK"
#define DFL_NOT_BEFORE          "20010101000000"
#define DFL_NOT_AFTER           "20301231235959"
#define DFL_SERIAL              "1"
#define DFL_SELFSIGN            1
#define DFL_IS_CA               0
#define DFL_MAX_PATHLEN         -1
#define DFL_KEY_USAGE           0
#define DFL_NS_CERT_TYPE        0

/*
USAGE_CSR                                           
    subject_key=%%s      default: subject.key 
    subject_pwd=%%s      default: (empty)     
    subject_name=%%s     default: CN=Cert,O=mbed TLS,C=UK
    
    issuer_crt=%%s       default: (empty)       
                        If issuer_crt is specified, issuer_name is  
                        ignored!              
    issuer_name=%%s      default: CN=CA,O=mbed TLS,C=UK     
                                                
    selfsign=%%d         default: 1 (true)     
                        If selfsign is enabled, issuer_name and
                        issuer_key are required (issuer_crt and
                        subject_* are ignored
    issuer_key=%%s       default: ca.key        
    issuer_pwd=%%s       default: (empty)     
    output_file=%%s      default: cert.crt     
    serial=%%s           default: 1            
    not_before=%%s       default: 20010101000000
    not_after=%%s        default: 20301231235959
    is_ca=%%d            default: 0 (disabled)  
    max_pathlen=%%d      default: -1 (none)     
    key_usage=%%s        default: (empty)       
                        Comma-separated-list of values:
                          digital_signature     
                          non_repudiation       
                          key_encipherment      
                          data_encipherment     
                          key_agreement         
                          key_cert_sign  
                          crl_sign            
    ns_cert_type=%%s     default: (empty)      
                        Comma-separated-list of values:     
                          ssl_client           
                          ssl_server           
                          email               
                          object_signing       
                          ssl_ca               
                          email_ca              
                          object_signing_ca  
*/
struct options_cert
{
    const char *issuer_crt;     /* filename of the issuer certificate   */
    const char *request_file;   /* filename of the certificate request  */
    const char *issuer_key;     /* filename of the issuer key file      */
    const char *subject_pwd;    /* password for the subject key file    */
    const char *issuer_pwd;     /* password for the issuer key file     */
    const char *output_file;    /* where to store the constructed key file  */
    const char *subject_name;   /* subject name for certificate         */
    const char *issuer_name;    /* issuer name for certificate          */
    const char *not_before;     /* validity period not before           */
    const char *not_after;      /* validity period not after            */
    const char *serial;         /* serial number string                 */
    int selfsign;               /* selfsign the certificate             */
    int is_ca;                  /* is a CA certificate                  */
    int max_pathlen;            /* maximum CA path length               */
    unsigned char key_usage;    /* key usage flags                      */
    unsigned char ns_cert_type; /* NS cert type                         */
} opt_cert;

static int write_certificate( mbedtls_x509write_cert *crt,
                       int (*f_rng)(void *, unsigned char *, size_t),
                       void *p_rng, unsigned char* output_buf )
{
    int ret ;
    if( (ret = mbedtls_x509write_crt_pem( crt, output_buf, 2048, f_rng, p_rng ) ) < 0 )
        return ret;
    return strlen((char *)output_buf);
}
static int create_certificate_from_key(unsigned char *cert_buf)
{ 
     
    int ret = 0;
    mbedtls_x509_crt issuer_crt;
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    mbedtls_x509_csr csr;
#endif
    mbedtls_x509write_cert crt;
    mbedtls_mpi serial;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    const char *pers = "crt example app";
    mbedtls_x509write_crt_init( &crt );
    mbedtls_x509write_crt_set_md_alg( &crt, MBEDTLS_MD_SHA256 );
    mbedtls_mpi_init( &serial );
    mbedtls_ctr_drbg_init( &ctr_drbg );
#if defined(MBEDTLS_X509_CSR_PARSE_C)
    mbedtls_x509_csr_init( &csr );
#endif
    mbedtls_x509_crt_init( &issuer_crt );
    mbedtls_entropy_init( &entropy );

    opt_cert.issuer_crt          = DFL_ISSUER_CRT;
    opt_cert.request_file        = DFL_REQUEST_FILE;
    opt_cert.issuer_key          = DFL_ISSUER_KEY;
    opt_cert.subject_pwd         = DFL_SUBJECT_PWD;
    opt_cert.issuer_pwd          = DFL_ISSUER_PWD;
    opt_cert.subject_name        = DFL_SUBJECT_NAME;
    opt_cert.issuer_name         = DFL_ISSUER_NAME;
    opt_cert.not_before          = DFL_NOT_BEFORE;
    opt_cert.not_after           = DFL_NOT_AFTER;
    opt_cert.serial              = DFL_SERIAL;
    opt_cert.selfsign            = DFL_SELFSIGN;
    opt_cert.is_ca               = DFL_IS_CA;
    opt_cert.max_pathlen         = DFL_MAX_PATHLEN;
    opt_cert.key_usage           = DFL_KEY_USAGE;
    opt_cert.ns_cert_type        = DFL_NS_CERT_TYPE;
    
    if( ( ret = mbedtls_ctr_drbg_seed( &ctr_drbg, mbedtls_entropy_func, &entropy,
                               (const unsigned char *) pers,
                               strlen( pers ) ) ) != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_ctr_drbg_seed\n\n");// returned %d \n", ret);//, buf );
        goto exit;
    }


    if( ( ret = mbedtls_mpi_read_string( &serial, 10, opt_cert.serial ) ) != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_mpi_read_string\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

    if( strlen( opt_cert.issuer_crt ) )
    {
        if( !mbedtls_pk_can_do( &issuer_crt.pk, MBEDTLS_PK_RSA ) ||
            mbedtls_mpi_cmp_mpi( &mbedtls_pk_rsa( issuer_crt.pk )->N,
                         &mbedtls_pk_rsa( global_key )->N ) != 0 ||
            mbedtls_mpi_cmp_mpi( &mbedtls_pk_rsa( issuer_crt.pk )->E,
                         &mbedtls_pk_rsa( global_key )->E ) != 0 )
        {
            mbedtls_printf( " failed\n  !  issuer_key does not match issuer certificate\n\n" );
            ret = -1;
            goto exit;
        }
    }


    if( opt_cert.selfsign )
    {
        opt_cert.subject_name = opt_cert.issuer_name;
    }

    mbedtls_x509write_crt_set_subject_key( &crt, &global_key );
    mbedtls_x509write_crt_set_issuer_key( &crt, &global_key );

    /*
     * 1.0. Check the names for validity
     */
    if( ( ret = mbedtls_x509write_crt_set_subject_name( &crt, opt_cert.subject_name ) ) != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_subject_name\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

    if( ( ret = mbedtls_x509write_crt_set_issuer_name( &crt, opt_cert.issuer_name ) ) != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_issuer_name\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

    ret = mbedtls_x509write_crt_set_serial( &crt, &serial );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_serial\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

    ret = mbedtls_x509write_crt_set_validity( &crt, opt_cert.not_before, opt_cert.not_after );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_validity\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

    ret = mbedtls_x509write_crt_set_basic_constraints( &crt, opt_cert.is_ca,
                                               opt_cert.max_pathlen );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  x509write_crt_set_basic_contraints\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }


#if defined(MBEDTLS_SHA1_C)

    ret = mbedtls_x509write_crt_set_subject_key_identifier( &crt );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_subject_key_identifier\n\n");// returned -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }


    ret = mbedtls_x509write_crt_set_authority_key_identifier( &crt );
    if( ret != 0 )
    {
        mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_authority_key_identifier returned\n\n");// -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

#endif /* MBEDTLS_SHA1_C */

    if( opt_cert.key_usage )
    {
        ret = mbedtls_x509write_crt_set_key_usage( &crt, opt_cert.key_usage );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_key_usage returned\n\n");// -0x%02x - %s\n\n", -ret, buf );
            goto exit;
        }
    }

    if( opt_cert.ns_cert_type )
    {
        ret = mbedtls_x509write_crt_set_ns_cert_type( &crt, opt_cert.ns_cert_type );
        if( ret != 0 )
        {
            mbedtls_printf( " failed\n  !  mbedtls_x509write_crt_set_ns_cert_type returned \n\n");//-0x%02x - %s\n\n", -ret, buf );
            goto exit;
        }
    }

    /*
     * 1.2. Writing the certificate 
     */
		
    if( ( ret = write_certificate( &crt, mbedtls_ctr_drbg_random, &ctr_drbg, cert_buf ) ) < 0  ) //memory consuming as fuck 
    {
        mbedtls_printf( " failed\n->!write_certifcate error code %d \n\n", ret );// -0x%02x - %s\n\n", -ret, buf );
        goto exit;
    }

exit:
    mbedtls_mpi_free( &serial );
    mbedtls_entropy_free( &entropy );
    mbedtls_x509write_crt_free( &crt );
    mbedtls_x509_crt_free(&issuer_crt);
    mbedtls_ctr_drbg_free( &ctr_drbg );
    mbedtls_x509_csr_free(&csr);
    mbedtls_pk_free(&global_key);
    return( ret );
}

typedef struct _machine_certificates_obj_t {
    mp_obj_base_t base;
} machine_certificates_obj_t;

STATIC void machine_certificates_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_printf(print, "Certificates()");
}

STATIC mp_obj_t machine_certificates_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_certificates_obj_t *self = m_new_obj(machine_certificates_obj_t);
    self->base.type = &machine_certificates_type;
    return MP_OBJ_FROM_PTR(self);
}

#define KEY_BUFF_SIZE 1024
STATIC mp_obj_t machine_certificates_generate(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    unsigned char *key_buff = m_malloc(KEY_BUFF_SIZE);
    if(key_buff == NULL) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Memory allocation failed."));
        return mp_const_none;
    }
	unsigned char *cert_buff = m_malloc(KEY_BUFF_SIZE);
    if(NULL == cert_buff) {
        m_free(key_buff);
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "Memory allocation failed."));
        return mp_const_none;
    }
	mp_obj_tuple_t *tuple_p;
	tuple_p = MP_OBJ_TO_PTR(mp_obj_new_tuple(2, NULL));
	int key_len=0, cert_len=0; 
	// create key
	memset(key_buff, 0, KEY_BUFF_SIZE);
	key_len = ssl_context_create_private_key(key_buff);
	memset(cert_buff,0, KEY_BUFF_SIZE );
	cert_len = create_certificate_from_key(cert_buff );
	tuple_p->items[0] = mp_obj_new_str((const char*) key_buff, key_len, false);
    m_free(key_buff);
	tuple_p->items[1] = mp_obj_new_str((const char*) cert_buff, cert_len, false);
    m_free(cert_buff);
	return (MP_OBJ_FROM_PTR(tuple_p));
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_certificates_generate_obj, 0, machine_certificates_generate);
STATIC const mp_rom_map_elem_t machine_certificates_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_generate), MP_ROM_PTR(&machine_certificates_generate_obj) },
};


STATIC MP_DEFINE_CONST_DICT(machine_certificates_locals_dict, machine_certificates_locals_dict_table);
const mp_obj_type_t machine_certificates_type = {
    { &mp_type_type },
    .name = MP_QSTR_Certificates,
    .print = machine_certificates_print,
    .make_new = machine_certificates_make_new,
    .locals_dict = (mp_obj_dict_t*)&machine_certificates_locals_dict,
};

