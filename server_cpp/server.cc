#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

#include <grpcpp/grpcpp.h>
#include "file_processor.grpc.pb.h" // Arquivo gerado pelo protobuf

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using file_processor::FileProcessorService;
using file_processor::FileRequest;
using file_processor::FileResponse;
using file_processor::FileChunk;

class FileProcessorServiceImpl final : public FileProcessorService::Service {
    public:
        Status CompressPDF(ServerContext* context, const FileRequest* request, FileResponse* response) override {
            std::string input_file_path = "/tmp/input_" + request->file_name(); //Arquivo temporário
            std::string output_file_path = "/tmp/output_" + request->file_name();
            std::ofstream input_file_stream(input_file_path, std::ios::binary);
            if (!input_file_stream) {
                LogError("CompressPDF", request->file_name(), "Falha ao criar arquivo temporário de entrada.");
                response->set_success(false);
                response->set_status_message("Erro no servidor ao criar arquivo temporário.");
                return Status::INTERNAL;
            }
            // Receber stream do cliente e salvar no arquivo temporário
            grpc::ClientReaderWriter<FileChunk, FileChunk>* stream = grpc::ServerContext::FromServerContext(*context).ReaderWriter();
            FileChunk chunk;
            while (stream->Read(&chunk)) {
                input_file_stream.write(chunk.content().c_str(),
                chunk.content().size());
            }
            input_file_stream.close();

            // Executar comando gs
            std::string command = "gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/ebook -dNOPAUSE -dQUIET -dBATCH -sOutputFile=" + output_file_path 
            + " " + input_file_path;
            int gs_result = std::system(command.c_str());
            
            if (gs_result == 0) {
                LogError("CompressPDF", request->file_name(), "Compressão PDF bem-sucedida.");
                response->set_success(true);
                response->set_file_name("compressed_" + request->file_name());
                std::ifstream output_file_stream(output_file_path, std::ios::binary);
            
            if (output_file_stream) {
                while (output_file_stream.peek() != EOF) {
                    FileChunk response_chunk;
                    char buffer[1024];
                    output_file_stream.read(buffer, sizeof(buffer));
                    response_chunk.set_content(buffer, output_file_stream.gcount());
                    stream->Write(response_chunk); // Enviar stream para o cliente

                }
                output_file_stream.close();
            } else {
                LogError("CompressPDF", request->file_name(), "Falha ao abrir arquivo comprimido para envio.");
                response->set_success(false);
                response->set_status_message("Erro no servidor ao abrir arquivo comprimido.");
                return Status::INTERNAL;
            }
            stream->WritesDone();
            stream->Finish();
            } else {
                LogError("CompressPDF", request->file_name(), "Falha na compressão PDF. Código de retorno: " + std::to_string(gs_result));
                response->set_success(false);
                response->set_status_message("Falha ao comprimir PDF.");
                return Status::INTERNAL;
            }
            std::remove(input_file_path.c_str()); // Limpar arquivos temporários
            std::remove(output_file_path.c_str());

            return Status::OK;
        }

    private:
        void LogError(const std::string& service_name, const std::string& file_name, const std::string& message) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm;
            localtime_r(&now_c, &now_tm);
            char timestamp[26];
            std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &now_tm);
            std::ofstream log_file("server.log", std::ios::app);
            
            if (log_file.is_open()) {
                log_file << "[" << timestamp << "] ERROR - Service: " <<
                service_name << ", File: " << file_name << ", Message: " << message <<
                std::endl;

                log_file.close();
            } else {
                std::cerr << "Falha ao abrir arquivo de log!" << std::endl;
            }
            std::cerr << "[" << timestamp << "] ERROR - Service: " << service_name
            << ", File: " << file_name << ", Message: " << message << std::endl; // Log para console também
        }
        
        // ... (Função Log para sucesso - LogSuccess, similar a LogError, mas para logs de sucesso)
        void LogSuccess(const std::string& service_name, const std::string& file_name, const std::string& message) {
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::tm now_tm;
            localtime_r(&now_c, &now_tm);
            char timestamp[26];
            std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &now_tm);
            std::ofstream log_file("server.log", std::ios::app);
            if (log_file.is_open()) {
                log_file << "[" << timestamp << "] SUCCESS - Service: " <<
                service_name << ", File: " << file_name << ", Message: " << message <<
                std::endl;
                log_file.close();
            } else {
                std::cerr << "Falha ao abrir arquivo de log!" << std::endl;
            }
            std::cout << "[" << timestamp << "] SUCCESS - Service: " << service_name
            << ", File: " << file_name << ", Message: " << message << std::endl; // Log para console também
        }
};

void RunServer() {
    std::string server_address("0.0.0.0:50051");
    FileProcessorServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Servidor gRPC ouvindo em " << server_address << std::endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}