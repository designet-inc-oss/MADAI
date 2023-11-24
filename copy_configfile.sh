#!/bin/bash

if [ ! -e /etc/madai ]; then
    mkdir /etc/madai
fi

cp ../madai.conf.default /etc/madai/
