import grpc
import file_processor_pb2
import file_processor_pb2_grpc

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

def run_client():
    with grpc.insecure_channel('localhost:50051') as channel:
        stub = file_processor_pb2_grpc.FileProcessorServiceStub(channel)
        input_pdf = "teste.pdf" # Substitua pelo caminho do seu arquivo PDF de teste
        compress_pdf(stub, input_pdf)

if __name__ == '__main__':
    run_client()