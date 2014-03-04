# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Chromium presubmit script for src/chrome/browser/extensions.

See http://dev.chromium.org/developers/how-tos/depottools/presubmit-scripts
for more details on the presubmit API built into gcl.
"""

def GetPreferredTrySlaves():
  return ['linux_chromeos']

def CheckChangeOnUpload(input_api, output_api):
  results = []
  results += input_api.canned_checks.CheckPatchFormatted(input_api, output_api)
  return results

