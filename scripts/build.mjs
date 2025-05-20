#!/usr/bin/env zx

import { $, quote } from 'zx';

$.quote = quote;

try {
  await $`yarn prebuildify --strip --napi --arch x64`;
} catch (err) {
  console.error('Build failed:', err);
  throw err;
}
