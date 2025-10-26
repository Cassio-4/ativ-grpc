#!/usr/bin/env bash

PDF_IN="teste.pdf"
IMG_IN="alan_turing.jpg"

python client.py --file_path teste.pdf --compress_pdf

python client.py --file_path teste.pdf --pdf_to_txt

python client.py --file_path alan_turing.jpg --image_resize

python client.py --file_path alan_turing.jpg --image_convert