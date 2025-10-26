#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>
#include <filesystem>

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
using file_processor::ImageStreamRequest;
using file_processor::ResizeImageRequest;

class FileProcessorServiceImpl final : public FileProcessorService::Service {
    public:
        struct ResizeParams {
            int width;
            int height;
            string filename;
            string format;
        };

        using fileStreamer = ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::FileChunk >;
        using imageStreamer = ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::ImageStreamRequest>;

        grpc::Status CompressPDF(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::FileChunk>* stream) {
            string temp_filename;
            string output_file_path;
            try {
                temp_filename = writeToTempFile(stream);
            } catch (const runtime_error& e) {
                logError("CompressPDF", e.what(), "Falha ao criar arquivo temporário de entrada.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao criar arq temporario");
            }
            if (!isPdf(temp_filename)) {
                logError("CompressPDF", temp_filename, "Arquivo não é um PDF válido.");
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Arquivo não é um PDF válido.");
            }

            output_file_path = "compressed_" + temp_filename;
            string command = "gs -sDEVICE=pdfwrite -dCompatibilityLevel=1.4 -dPDFSETTINGS=/ebook -dNOPAUSE -dQUIET -dBATCH -sOutputFile="
            + output_file_path 
            + " " + temp_filename;
            
            int gs_result = system(command.c_str());
            
            if (gs_result != 0) {
                logError("CompressPDF", output_file_path, "Falha na compressão PDF.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao comprimir PDF");
            }
            try {
                writeToStream(stream, output_file_path);
            } catch (const runtime_error& e) {
                logError("CompressPDF", output_file_path, "Falha no envio do arquivo comprimido.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao enviar arq final");
            }
            
            remove(temp_filename.c_str());
            remove(output_file_path.c_str());
            logSuccess("CompressPDF", output_file_path, "Compressão PDF bem-sucedida.");
            return Status::OK;
        }

        grpc::Status ConvertToTXT(::grpc::ServerContext* context, ::grpc::ServerReaderWriter<::file_processor::FileChunk, ::file_processor::FileChunk>* stream) {
            string temp_filename;
            string output_file_path;
            try {
                temp_filename = writeToTempFile(stream);
            } catch (const runtime_error& e) {
                logError("ConvertToTXT", e.what(), "Falha ao criar arquivo temporário de entrada.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao criar arq temporario");
            }
            if (!isPdf(temp_filename)) {
                logError("ConvertToTXT", temp_filename, "Arquivo não é um PDF válido.");
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Arquivo não é um PDF válido.");
            }
            output_file_path = filesystem::path(temp_filename).replace_extension("txt").string();
            string command = "gs -sDEVICE=txtwrite -dNOPAUSE -dBATCH -dQUIET -sOutputFile="
            + output_file_path 
            + " " + temp_filename;
            
            int gs_result = system(command.c_str());
            
            if (gs_result != 0) {
                logError("ConvertToTXT", output_file_path, "Falha na conversão PDF.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao converter PDF");
            }
            try {
                writeToStream(stream, output_file_path);
            } catch (const runtime_error& e) {
                logError("ConvertToTXT", output_file_path, "Falha no envio do arquivo de texto.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao enviar arq final");
            }
            
            remove(temp_filename.c_str());
            remove(output_file_path.c_str());
            logSuccess("ConvertToTXT", output_file_path, "Conversão PDF -> TXT bem-sucedida.");
            return Status::OK;
        }

        grpc::Status ConvertImageFormat(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::ImageStreamRequest>* stream) {
            string temp_filename;
            string output_file_path;
            ResizeParams params;

            try {
                params = processImgChunks(stream);
            } catch (const runtime_error& e) {
                logError("ConvertImageFormat", e.what(), "Falha ao criar arquivo temporário de entrada.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao criar arq temporario");
            }

            temp_filename = params.filename;
            output_file_path = filesystem::path(temp_filename).replace_extension(params.format).string();
            string command = "convert "+ params.filename + " " + output_file_path;
            
            int gs_result = system(command.c_str());
            
            if (gs_result != 0) {
                logError("ConvertImageFormat", output_file_path, "Falha na conversão de Imagem.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro na conversão de imagem.");
            }
            
            try {
                writeImgToStream(stream, output_file_path);
            } catch (const runtime_error& e) {
                logError("ConvertImageFormat", output_file_path, "Falha no envio da imagem convertida.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao enviar img final");
            }
            
            remove(temp_filename.c_str());
            remove(output_file_path.c_str());
            logSuccess("ConvertImageFormat", output_file_path, "Conversão de imagem bem-sucedida.");
            return Status::OK;
        }
    
        grpc::Status ResizeImage(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::file_processor::FileChunk, ::file_processor::ImageStreamRequest>* stream) {
            string temp_filename;
            string output_file_path;
            ResizeParams params;
            try {
                params = processImgChunks(stream);
            } catch (const runtime_error& e) {
                logError("ResizeImage", e.what(), "Falha ao criar arquivo temporário de entrada.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao criar arq temporario");
            }
            
            output_file_path = "resized_" + params.filename;
            string command = "convert "+ params.filename+ " -resize " + to_string(params.width)+"x"+
            to_string(params.height)+" "+ output_file_path;
            
            int gs_result = system(command.c_str());
            
            if (gs_result != 0) {
                logError("ResizeImage", output_file_path, "Falha no redimensionamento de Imagem.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no redimensionamento de imagem.");
            }
            try {
                writeImgToStream(stream, output_file_path);
            } catch (const runtime_error& e) {
                logError("ResizeImage", output_file_path, "Falha no envio da imagem redimensionada.");
                return grpc::Status(grpc::StatusCode::INTERNAL, "Erro no sv ao enviar img final");
            }
            
            remove(temp_filename.c_str());
            remove(output_file_path.c_str());
            logSuccess("ResizeImage", output_file_path, "Redimensionamento de imagem bem-sucedida.");
            return Status::OK;
        }

    private:
        string writeToTempFile(fileStreamer* stream) {
            FileChunk chunk;
            string input_file_path = "empty";
            
            stream->Read(&chunk);
            string filename = chunk.filename();
            input_file_path = filename;
          
            ofstream input_file_stream(input_file_path, ios::binary);
            if (!input_file_stream) {
                throw runtime_error(input_file_path);
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

        ResizeParams processImgChunks(imageStreamer* stream) {
            ImageStreamRequest request;
            ResizeParams params;
            string input_file_path = "empty";
            
            stream->Read(&request);
            
            if (request.has_metadata()) {
                const ResizeImageRequest& metadata = request.metadata();
                params.width = metadata.width();
                params.height = metadata.height();
            } else if(request.has_format()){
                params.format = request.format();
            } else {
                throw runtime_error(input_file_path + " Metadados");
            }
            stream->Read(&request);
            if (request.has_chunk()) {
                const FileChunk& chunk = request.chunk();
                params.filename = chunk.filename();
                input_file_path = params.filename;
            }

            ofstream input_file_stream(params.filename, ios::binary);
            if (!input_file_stream) {
                throw runtime_error(input_file_path);
            }
            while (stream->Read(&request)) {
                const FileChunk& chunk = request.chunk();
                input_file_stream.write(chunk.content().c_str(), chunk.content().size());
            }
            input_file_stream.close();
            return params;
        }

        void writeImgToStream(imageStreamer* stream, const string result_filename) {
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

        bool isPdf(const std::string& filename) {
            filesystem::path file_path(filename);
            string extension = file_path.extension().string();
            
            // Convert to lowercase for case-insensitive comparison
            std::string lower_extension = extension;
            std::transform(lower_extension.begin(), lower_extension.end(), lower_extension.begin(), ::tolower);
            
            return lower_extension == ".pdf";
        }

        void logError(const std::string& service_name, const std::string& file_name, const std::string& message) {
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
        
        void logSuccess(const std::string& service_name, const std::string& file_name, const std::string& message) {
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