// Copyright 2019 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use std::path::Path;
use std::process::Command;

fn main() {
    let gen_file = Path::new("./src/gen.rs");
    if gen_file.exists() {
        return;
    }
    let header_dir = Path::new("../../src/common");
    let output = Command::new("bindgen")
        .arg(header_dir.join("cras_bindgen.h").to_str().unwrap())
        .args(&["--whitelist-type", "cras_.*"])
        .args(&["--whitelist-var", "cras_.*"])
        .args(&["--whitelist-type", "CRAS_.*"])
        .args(&["--whitelist-var", "CRAS_.*"])
        .args(&["--whitelist-type", "audio_message"])
        .args(&["--whitelist-var", "MAX_DEBUG_.*"])
        .args(&["--rustified-enum", "CRAS_.*"])
        .args(&["--rustified-enum", "_snd_pcm_.*"])
        .args(&["--bitfield-enum", "CRAS_STREAM_EFFECT"])
        .args(&["--output", gen_file.to_str().unwrap()])
        .output()
        .expect("Failed in bindgen command.");
    assert!(output.status.success(), "Got error from bindgen command");
}