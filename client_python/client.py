import grpc
import file_processor_pb2
import file_processor_pb2_grpc
import argparse

def compress_pdf(stub, input_file_path):
    def file_iterator():
        # First yield the filename
        filename = input_file_path.split('/')[-1]
        yield file_processor_pb2.FileChunk(
            filename=filename,
            content=b'',  # Empty content for the first message with filename
            success=False,
            is_last_chunk=False
        )
        
        # Then yield the file content in chunks
        with open(input_file_path, 'rb') as f:
            while True:
                chunk = f.read(1024)
                if not chunk:
                    # Last chunk with no content but is_last_chunk=True
                    yield file_processor_pb2.FileChunk(
                        content=b'',
                        success=False,
                        is_last_chunk=True
                    )
                    break
                
                yield file_processor_pb2.FileChunk(
                    content=chunk,
                    success=False,
                    is_last_chunk=False
                )
    
    # Use the generator directly in the RPC call
    response_stream = stub.CompressPDF(file_iterator())
    try:
        received_file_name = next(response_stream).filename
        with open(received_file_name, 'wb') as output_file:
            for chunk in response_stream:
                output_file.write(chunk.content)
        print(f"PDF comprimido e salvo em: {received_file_name}")
    except grpc.RpcError as e:
        print(f"Erro ao comprimir PDF: {e.details()}")
    except Exception as e:
        print(f"Erro ao salvar arquivo comprimido: {e}")

def pdf_to_txt(stub, input_file_path):
    def file_iterator():
        # First yield the filename
        filename = input_file_path.split('/')[-1]
        yield file_processor_pb2.FileChunk(
            filename=filename,
            content=b'',  # Empty content for the first message with filename
            success=False,
            is_last_chunk=False
        )
        
        # Then yield the file content in chunks
        with open(input_file_path, 'rb') as f:
            while True:
                chunk = f.read(1024)
                if not chunk:
                    # Last chunk with no content but is_last_chunk=True
                    yield file_processor_pb2.FileChunk(
                        content=b'',
                        success=False,
                        is_last_chunk=True
                    )
                    break
                
                yield file_processor_pb2.FileChunk(
                    content=chunk,
                    success=False,
                    is_last_chunk=False
                )
    
    # Use the generator directly in the RPC call
    response_stream = stub.ConvertToTXT(file_iterator())
    try:
        received_file_name = next(response_stream).filename
        with open(received_file_name, 'wb') as output_file:
            for chunk in response_stream:
                output_file.write(chunk.content)
        print(f"PDF convertido e salvo em: {received_file_name}")
    except grpc.RpcError as e:
        print(f"Erro ao processor PDF: {e.details()}")
    except Exception as e:
        print(f"Erro ao salvar arquivo de texto: {e}")

def run_client(args):
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = file_processor_pb2_grpc.FileProcessorServiceStub(channel)
        if args.compress_pdf:
            compress_pdf(stub, args.file_path)
        elif args.pdf_to_txt:
            pdf_to_txt(stub, args.file_path)
        elif args.image_resize:
            image_resize(stub, args.file_path, args.h, args.w)
        elif args.image_convert:
            image_convert(stub, args.file_path, args.format)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Run a Remote call on a file processing server.")
    parser.add_argument("--file_path", type=str, default="teste.pdf",
        help="Specify the path to the file to be processed."
    )
    parser.add_argument("--compress_pdf", action='store_true', default=False,
        help="If set, the client will send --file_path to the server to be compressed"
    )
    parser.add_argument("--pdf_to_txt", action='store_true', default=False,
        help="if set, the client will send --file_path to the server to be converted from PDF to TXT"
    )
    parser.add_argument("--image_resize", action='store_true', default=False,
        help="if set, the client will send --file_path to the server to be resized to --h --w."
    )
    parser.add_argument("--image_convert", action='store_true', default=False,
        help="if set, the client will send --file_path to the server to be converted to --format."
    )
    parser.add_argument("--h", type=int, default=100,
        help="set height for image resizing."
    )
    parser.add_argument("--w", type=int, default=100,
        help="set width for image resizing."
    )
    parser.add_argument("--format", type=str, default='png',
        help="set format for image conversion."
    )
    args = parser.parse_args()
    run_client(args)