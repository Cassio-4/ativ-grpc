#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

#include <grpcpp/grpcpp.h>
#include "proto/file_processor.grpc.pb.h"

using namespace std;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;
using file_processor::FileProcessorService;
using file_processor::FileChunk;

class FileProcessorServiceImpl final : public FileProcessorService::Service {
    public:
        using fileStreamer = ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::FileChunk >;
        
        grpc::Status CompressPDF(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::FileChunk>* stream) {
            std::string temp_filename;
            std::string output_file_path;
            try {
                temp_filename = writeToTempFile(stream);
            } catch (const runtime_error& e) {
                //response->set_success(false);
                //response->set_status_message("Erro no servidor ao criar arquivo temporário.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao criar arq temporario");
            }
            output_file_path = "compressed_" + temp_filename;
            std::string command = "gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/ebook -dNOPAUSE -dQUIET -dBATCH -sOutputFile="
            + output_file_path 
            + " " + temp_filename;
            
            int gs_result = system(command.c_str());
            
            if (gs_result == 0) {
                //LogError("CompressPDF", request->file_name(), "Compressão PDF bem-sucedida.");
                //response->set_success(true);
                //response->set_file_name("compressed_" + request->file_name());
                cout << "Compressão PDF bem-sucedida." << endl;
            }
            try {
                writeToStream(stream, output_file_path);
            } catch (const runtime_error& e) {
                //LogError("CompressPDF", request->file_name(), "Falha ao abrir arquivo comprimido para envio.");
                //response->set_success(false);
                //response->set_status_message("Erro no servidor ao criar arquivo temporário.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao criar arq temporario");
            }
            
            // } else {
            //     LogError("CompressPDF", request->file_name(), "Falha na compressão PDF. Código de retorno: " + std::to_string(gs_result));
            //     response->set_success(false);
            //     response->set_status_message("Falha ao comprimir PDF.");
            //     return Status::INTERNAL;
            // }
            remove(temp_filename.c_str());
            remove(output_file_path.c_str());
            return Status::OK;
        }
    private:
        string writeToTempFile(fileStreamer* stream) {
            FileChunk chunk;
            stream->Read(&chunk);
            string filename = chunk.filename();
            string input_file_path = filename;
            cout << "Criando arquivo temporário de entrada: " << input_file_path << endl;
            ofstream input_file_stream(input_file_path, ios::binary);
            if (!input_file_stream) {
                //LogError("CompressPDF", filename, "Falha ao criar arquivo temporário de entrada.");
                cout << "Erro ao criar arquivo temporário de entrada." << endl;
                throw runtime_error("Erro ao criar arquivo temporário de entrada.");
            }
            while (stream->Read(&chunk)) {
                input_file_stream.write(chunk.content().c_str(), chunk.content().size());
            }
            input_file_stream.close();
            return filename;
        }

        void writeToStream(fileStreamer* stream, const string result_filename) {
            ifstream file_to_stream(result_filename, ios::binary);
            if (file_to_stream) {
                
                FileChunk chunk;
                chunk.set_filename(result_filename);
                chunk.set_success(true);
                chunk.set_is_last_chunk(false);
                chunk.set_content("");
                stream->Write(chunk); // Enviar metadados iniciais
                
                while (true) {
                    char buffer[1024];
                    file_to_stream.read(buffer, sizeof(buffer));
                    chunk.set_content(buffer, file_to_stream.gcount());
                    if (file_to_stream.peek() == EOF) {
                        chunk.set_is_last_chunk(true);
                    }
                    stream->Write(chunk); // Enviar stream para o cliente
                    if (file_to_stream.peek() == EOF) {
                       break;
                    }
                }
                file_to_stream.close();
            } else {
                throw runtime_error("Erro ao abrir arquivo para streaming.");
            }
        }
//         void LogError(const std::string& service_name, const std::string& file_name, const std::string& message) {
//             auto now = std::chrono::system_clock::now();
//             std::time_t now_c = std::chrono::system_clock::to_time_t(now);
//             std::tm now_tm;
//             localtime_r(&now_c, &now_tm);
//             char timestamp[26];
//             std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &now_tm);
//             std::ofstream log_file("server.log", std::ios::app);
            
//             if (log_file.is_open()) {
//                 log_file << "[" << timestamp << "] ERROR - Service: " <<
//                 service_name << ", File: " << file_name << ", Message: " << message <<
//                 std::endl;

//                 log_file.close();
//             } else {
//                 std::cerr << "Falha ao abrir arquivo de log!" << std::endl;
//             }
//             std::cerr << "[" << timestamp << "] ERROR - Service: " << service_name
//             << ", File: " << file_name << ", Message: " << message << std::endl; // Log para console também
//         }
        
//         // ... (Função Log para sucesso - LogSuccess, similar a LogError, mas para logs de sucesso)
//         void LogSuccess(const std::string& service_name, const std::string& file_name, const std::string& message) {
//             auto now = std::chrono::system_clock::now();
//             std::time_t now_c = std::chrono::system_clock::to_time_t(now);
//             std::tm now_tm;
//             localtime_r(&now_c, &now_tm);
//             char timestamp[26];
//             std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &now_tm);
//             std::ofstream log_file("server.log", std::ios::app);
//             if (log_file.is_open()) {
//                 log_file << "[" << timestamp << "] SUCCESS - Service: " <<
//                 service_name << ", File: " << file_name << ", Message: " << message <<
//                 std::endl;
//                 log_file.close();
//             } else {
//                 std::cerr << "Falha ao abrir arquivo de log!" << std::endl;
//             }
//             std::cout << "[" << timestamp << "] SUCCESS - Service: " << service_name
//             << ", File: " << file_name << ", Message: " << message << std::endl; // Log para console também
//         }
};

void RunServer() {
    string server_address("0.0.0.0:50051");
    FileProcessorServiceImpl service;
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    unique_ptr<Server> server(builder.BuildAndStart());
    cout << "Servidor gRPC ouvindo em " << server_address << endl;
    server->Wait();
}

int main() {
    RunServer();
    return 0;
}