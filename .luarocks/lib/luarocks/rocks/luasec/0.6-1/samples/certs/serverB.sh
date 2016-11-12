#!/bin/sh

openssl req -newkey rsa:1024 -keyout serverBkey.pem -out serverBreq.pem \
   -config ./serverB.cnf -nodes -days 365 -batch

openssl x509 -req -in serverBreq.pem -sha1 -extfile ./serverB.cnf \
   -extensions usr_cert -CA rootB.pem -CAkey rootBkey.pem -CAcreateserial \
   -out serverBcert.pem -days 365

cat serverBcert.pem rootB.pem > serverB.pem

openssl x509 -subject -issuer -noout -in serverB.pem
