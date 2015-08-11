#!/bin/sh
echo $(git describe --tags)$(git status --porcelain | awk '{if ($1 == "M") {print "-dirty";exit}}') | tr -d '\n'
