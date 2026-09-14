# Example 05: Multipart File Upload

Demonstrates how to accept, inspect, sanitize, and save binary file uploads using RFC 7578 `multipart/form-data`.

---

## Features Demonstrated

- Receiving file uploads via `req.files` (`std::vector<UploadedFile>`).
- Inspecting upload metadata (`file.filename`, `file.content_type`, `file.size()`).
- Saving files to disk with binary fidelity via `file.save(destination)`.
- **Path Traversal Defense (CWE-22)**: Sanitizing filenames to prevent malicious payloads like `../../etc/passwd` or `..\..\windows\win.ini`.
- Returning JSON status reports summarizing uploaded files.

---

## Build & Run

### Linux & macOS (GCC / Clang)

From the project root:

```bash
g++ -std=c++11 -I include examples/05_file_upload/main.cpp -lssl -lcrypto -lpthread -o file_upload
./file_upload
```

### Windows (MinGW / GCC)

```powershell
g++ -std=c++11 -I include examples/05_file_upload/main.cpp -lssl -lcrypto -lpthread -lws2_32 -o file_upload.exe
.\file_upload.exe
```

---

## Testing the Uploads

### Option A: Web Browser
1. Open `http://127.0.0.1:8080/` in your browser.
2. Choose any file (image, PDF, text file) and click **Upload File**.
3. View the JSON response confirming the file name, MIME type, byte size, and disk location.
4. Verify the saved file exists in the `uploads/` directory.

### Option B: cURL Command Line
```bash
# Create a sample text file
echo "Hello from Peregrine upload test!" > sample.txt

# Upload via cURL
curl -i -F "file=@sample.txt" http://127.0.0.1:8080/upload

# Clean up local sample
rm sample.txt
```
