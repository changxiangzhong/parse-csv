#ifndef SQL_OP_H
#define SQL_OP_H
#include <mysql/mysql.h>
#define MYSQL_USERNAME "chang"
#define MYSQL_PASSWORD "111111"
#define MYSQL_HOSTNAME "localhost"
#define MYSQL_SCHEMA "accounting"

#define MYSQL_DDL "create table if not exists WC05101 (company varchar(128), year date, value double);"
#endif
