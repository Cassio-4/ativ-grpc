#!/usr/bin/env bash

# Test inputs (must exist in repo root or adjust paths)
PDF_IN="teste.pdf"
IMG_IN="alan_turing.jpg"
OUT_DIR="./test_outputs"
PYTHON=${PYTHON:-python3}

if [ ! -f "$PDF_IN" ]; then
  echo "Missing $PDF_IN"
  exit 1
fi
if [ ! -f "$IMG_IN" ]; then
  echo "Missing $IMG_IN"
  exit 1
fi

echo "1) Compress PDF using client.py"
$PYTHON client_python/client.py --file_path "$PDF_IN" --compress_pdf || {
  echo "Compress PDF failed"
}

echo "2) Convert PDF to TXT using client.py"
$PYTHON client_python/client.py --file_path "$PDF_IN" --pdf_to_txt || {
  echo "Convert PDF->TXT failed"
}

