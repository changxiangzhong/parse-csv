#ifndef SQL_OP_H
#define SQL_OP_H
#include <mysql/mysql.h>
#include <mysql/mysql_time.h>
#define MYSQL_USERNAME "chang"
#define MYSQL_PASSWORD "111111"
#define MYSQL_HOSTNAME "localhost"
#define MYSQL_SCHEMA "accounting"
#define MYSQL_COMPANY_NAME_LEN 128
#define MYSQL_CREATE_TABLE "create table if not exists WC05101 (company varchar(128) NOT NULL CHECK(company <> ''), year date not null, value double not null)"
#define MYSQL_INSERT "insert into WC05101 values (?, ?, ?)"
#endif
