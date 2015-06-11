/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "command.h"
#include "environment.h"
#include "record.h"
#include "record_file.h"

using namespace PerfFileFormat;

static std::unique_ptr<Command> RecordCmd() {
  return CreateCommandInstance("record");
}

TEST(record_cmd, no_options) {
  ASSERT_TRUE(RecordCmd()->Run({"sleep", "1"}));
}

TEST(record_cmd, system_wide_option) {
  ASSERT_TRUE(RecordCmd()->Run({"-a", "sleep", "1"}));
}

TEST(record_cmd, sample_period_option) {
  ASSERT_TRUE(RecordCmd()->Run({"-c", "100000", "sleep", "1"}));
}

TEST(record_cmd, event_option) {
  ASSERT_TRUE(RecordCmd()->Run({"-e", "cpu-clock", "sleep", "1"}));
}

TEST(record_cmd, freq_option) {
  ASSERT_TRUE(RecordCmd()->Run({"-f", "99", "sleep", "1"}));
  ASSERT_TRUE(RecordCmd()->Run({"-F", "99", "sleep", "1"}));
}

TEST(record_cmd, output_file_option) {
  ASSERT_TRUE(RecordCmd()->Run({"-o", "perf2.data", "sleep", "1"}));
}

TEST(record_cmd, dump_kernel_mmap) {
  ASSERT_TRUE(RecordCmd()->Run({"sleep", "1"}));
  std::unique_ptr<RecordFileReader> reader = RecordFileReader::CreateInstance("perf.data");
  ASSERT_TRUE(reader != nullptr);
  std::vector<std::unique_ptr<const Record>> records = reader->DataSection();
  ASSERT_GT(records.size(), 0U);
  bool have_kernel_mmap = false;
  for (auto& record : records) {
    if (record->header.type == PERF_RECORD_MMAP) {
      const MmapRecord* mmap_record = static_cast<const MmapRecord*>(record.get());
      if (mmap_record->filename == DEFAULT_KERNEL_MMAP_NAME) {
        have_kernel_mmap = true;
        break;
      }
    }
  }
  ASSERT_TRUE(have_kernel_mmap);
}

TEST(record_cmd, dump_build_id_feature) {
  ASSERT_TRUE(RecordCmd()->Run({"sleep", "1"}));
  std::unique_ptr<RecordFileReader> reader = RecordFileReader::CreateInstance("perf.data");
  ASSERT_TRUE(reader != nullptr);
  const FileHeader* file_header = reader->FileHeader();
  ASSERT_TRUE(file_header != nullptr);
  ASSERT_TRUE(file_header->features[FEAT_BUILD_ID / 8] & (1 << (FEAT_BUILD_ID % 8)));
  ASSERT_GT(reader->FeatureSectionDescriptors().size(), 0u);
}

TEST(record_cmd, tracepoint_event) {
  ASSERT_TRUE(RecordCmd()->Run({"-a", "-e", "sched:sched_switch", "sleep", "1"}));
}

extern bool IsBranchSamplingSupported();

TEST(record_cmd, branch_sampling) {
  if (IsBranchSamplingSupported()) {
    ASSERT_TRUE(RecordCmd()->Run({"-a", "-b", "sleep", "1"}));
    ASSERT_TRUE(RecordCmd()->Run({"-j", "any,any_call,any_ret,ind_call", "sleep", "1"}));
    ASSERT_TRUE(RecordCmd()->Run({"-j", "any,k", "sleep", "1"}));
    ASSERT_TRUE(RecordCmd()->Run({"-j", "any,u", "sleep", "1"}));
    ASSERT_FALSE(RecordCmd()->Run({"-j", "u", "sleep", "1"}));
  } else {
    GTEST_LOG_(INFO)
        << "This test does nothing as branch stack sampling is not supported on this device.";
  }
}
