#ifndef FASTDHT_CLIENT_H
#define FASTDHT_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

PHP_MINIT_FUNCTION(fastdht_client);
PHP_RINIT_FUNCTION(fastdht_client);
PHP_MSHUTDOWN_FUNCTION(fastdht_client);
PHP_RSHUTDOWN_FUNCTION(fastdht_client);
PHP_MINFO_FUNCTION(fastdht_client);

ZEND_FUNCTION(fastdht_set);
ZEND_FUNCTION(fastdht_get);
ZEND_FUNCTION(fastdht_inc);
ZEND_FUNCTION(fastdht_delete);
ZEND_FUNCTION(fastdht_batch_set);
ZEND_FUNCTION(fastdht_batch_get);
ZEND_FUNCTION(fastdht_batch_delete);

#ifdef __cplusplus
}
#endif

#endif	/* FASTDHT_CLIENT_H */