#!/usr/bin/env zx

import { $, quote } from 'zx';

$.quote = quote;

await $`yarn prebuildify --strip --napi --arch x64`;
