// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GOOGLE_APIS_DRIVE_UPLOADER_H_
#define CHROME_BROWSER_GOOGLE_APIS_DRIVE_UPLOADER_H_

#include <string>

#include "base/basictypes.h"
#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/google_apis/drive_service_interface.h"
#include "chrome/browser/google_apis/gdata_errorcode.h"
#include "chrome/browser/google_apis/gdata_wapi_parser.h"

class GURL;

namespace base {
class FilePath;
}

namespace google_apis {
class DriveServiceInterface;
struct UploadRangeResponse;

// Callback to be invoked once the upload has completed.
// |upload_location| will be returned when the uploading process is started but
// terminated before the completion due to some errors. It can be used to
// resume it.
typedef base::Callback<void(GDataErrorCode error,
                            const GURL& upload_location,
                            scoped_ptr<ResourceEntry> resource_entry)>
    UploadCompletionCallback;

class DriveUploaderInterface {
 public:
  virtual ~DriveUploaderInterface() {}

  // Uploads a new file to a directory specified by |upload_location|.
  // Returns the upload_id.
  //
  // parent_resource_id:
  //   resource id of the destination directory.
  //
  // drive_file_path:
  //   The destination path like "drive/foo/bar.txt".
  //
  // local_file_path:
  //   The path to the local file to be uploaded.
  //
  // title:
  //   The title (file name) of the file to be uploaded.
  //
  // content_type:
  //   The content type of the file to be uploaded.
  //
  // callback:
  //   Called when an upload is done regardless of it was successful or not.
  //   Must not be null.
  //
  // progress_callback:
  //   Periodically called back with the total number of bytes sent so far.
  //   May be null if the information is not needed.
  virtual void UploadNewFile(const std::string& parent_resource_id,
                             const base::FilePath& drive_file_path,
                             const base::FilePath& local_file_path,
                             const std::string& title,
                             const std::string& content_type,
                             const UploadCompletionCallback& callback,
                             const ProgressCallback& progress_callback) = 0;

  // Uploads an existing file (a file that already exists on Drive).
  //
  // See comments at UploadNewFile() about common parameters.
  //
  // resource_id:
  //   resource id of the existing file to be overwritten.
  //
  // etag:
  //   Expected ETag for the destination file. If it does not match, the upload
  //   fails with UPLOAD_ERROR_CONFLICT.
  //   If |etag| is empty, the test is skipped.
  virtual void UploadExistingFile(
      const std::string& resource_id,
      const base::FilePath& drive_file_path,
      const base::FilePath& local_file_path,
      const std::string& content_type,
      const std::string& etag,
      const UploadCompletionCallback& callback,
      const ProgressCallback& progress_callback) = 0;

  // Resumes the uploading process termineted before the completion.
  // |upload_location| should be the one returned via UploadCompletionCallback
  // for previous invocation. |drive_file_path|, |local_file_path| and
  // |content_type| must be set to the same ones for previous invocation.
  //
  // See comments for UploadNewFile() about common parameters.
  virtual void ResumeUploadFile(
      const GURL& upload_location,
      const base::FilePath& drive_file_path,
      const base::FilePath& local_file_path,
      const std::string& content_type,
      const UploadCompletionCallback& callback,
      const ProgressCallback& progress_callback) = 0;
};

class DriveUploader : public DriveUploaderInterface {
 public:
  explicit DriveUploader(DriveServiceInterface* drive_service);
  virtual ~DriveUploader();

  // DriveUploaderInterface overrides.
  virtual void UploadNewFile(
      const std::string& parent_resource_id,
      const base::FilePath& drive_file_path,
      const base::FilePath& local_file_path,
      const std::string& title,
      const std::string& content_type,
      const UploadCompletionCallback& callback,
      const ProgressCallback& progress_callback) OVERRIDE;
  virtual void UploadExistingFile(
      const std::string& resource_id,
      const base::FilePath& drive_file_path,
      const base::FilePath& local_file_path,
      const std::string& content_type,
      const std::string& etag,
      const UploadCompletionCallback& callback,
      const ProgressCallback& progress_callback) OVERRIDE;
  virtual void ResumeUploadFile(
      const GURL& upload_location,
      const base::FilePath& drive_file_path,
      const base::FilePath& local_file_path,
      const std::string& content_type,
      const UploadCompletionCallback& callback,
      const ProgressCallback& progress_callback) OVERRIDE;

 private:
  struct UploadFileInfo;
  typedef base::Callback<void(scoped_ptr<UploadFileInfo> upload_file_info)>
      StartInitiateUploadCallback;

  // Starts uploading a file with |upload_file_info|.
  void StartUploadFile(
      scoped_ptr<UploadFileInfo> upload_file_info,
      const StartInitiateUploadCallback& start_initiate_upload_callback);
  void StartUploadFileAfterGetFileSize(
      scoped_ptr<UploadFileInfo> upload_file_info,
      const StartInitiateUploadCallback& start_initiate_upload_callback,
      bool get_file_size_result);

  // Starts to initate the new file uploading.
  // Upon completion, OnUploadLocationReceived should be called.
  void StartInitiateUploadNewFile(
      const std::string& parent_resource_id,
      const std::string& title,
      scoped_ptr<UploadFileInfo> upload_file_info);

  // Starts to initate the existing file uploading.
  // Upon completion, OnUploadLocationReceived should be called.
  void StartInitiateUploadExistingFile(
      const std::string& resource_id,
      const std::string& etag,
      scoped_ptr<UploadFileInfo> upload_file_info);

  // DriveService callback for InitiateUpload.
  void OnUploadLocationReceived(scoped_ptr<UploadFileInfo> upload_file_info,
                                GDataErrorCode code,
                                const GURL& upload_location);

  // Starts to get the current upload status for the file uploading.
  // Upon completion, OnUploadRangeResponseReceived should be called.
  void StartGetUploadStatus(scoped_ptr<UploadFileInfo> upload_file_info);

  // Uploads the next chunk of data from the file.
  void UploadNextChunk(scoped_ptr<UploadFileInfo> upload_file_info,
                       int64 start_position);

  // DriveService callback for ResumeUpload.
  void OnUploadRangeResponseReceived(
      scoped_ptr<UploadFileInfo> upload_file_info,
      const UploadRangeResponse& response,
      scoped_ptr<ResourceEntry> entry);
  void OnUploadProgress(const ProgressCallback& callback,
                        int64 start_position,
                        int64 total_size,
                        int64 progress_of_chunk,
                        int64 total_of_chunk);

  // Handle failed uploads.
  void UploadFailed(scoped_ptr<UploadFileInfo> upload_file_info,
                    GDataErrorCode error);

  // The lifetime of this object should be guaranteed to exceed that of the
  // DriveUploader instance.
  DriveServiceInterface* drive_service_;  // Not owned by this class.

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<DriveUploader> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DriveUploader);
};

}  // namespace google_apis

#endif  // CHROME_BROWSER_GOOGLE_APIS_DRIVE_UPLOADER_H_
