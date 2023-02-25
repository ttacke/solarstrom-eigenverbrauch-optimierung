#!/usr/bin/env bash

docker run --rm --name solar-eigenverbrauch-optimierung-webdev -v `pwd`/../sd-karteninhalt:/usr/share/nginx/html:ro -p 80:80 nginx
