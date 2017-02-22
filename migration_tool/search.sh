#!/bin/bash

grep -n $1 *.c *.h perfmon/* thread_migration/* page_migration/* --color=always
