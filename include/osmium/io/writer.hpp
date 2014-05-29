#ifndef OSMIUM_IO_WRITER_HPP
#define OSMIUM_IO_WRITER_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <future>
#include <memory>
#include <string>
#include <utility>

#include <osmium/io/compression.hpp>
#include <osmium/io/detail/output_format.hpp>
#include <osmium/io/detail/read_write.hpp>
#include <osmium/io/file.hpp>
#include <osmium/io/header.hpp>
#include <osmium/thread/checked_task.hpp>
#include <osmium/thread/name.hpp>

namespace osmium {

    namespace io {

        class OutputThread {

            typedef osmium::io::detail::data_queue_type data_queue_type;

            data_queue_type& m_input_queue;
            osmium::io::Compressor* m_compressor;

        public:

            OutputThread(data_queue_type& input_queue, osmium::io::Compressor* compressor) :
                m_input_queue(input_queue),
                m_compressor(compressor) {
            }

            void operator()() {
                osmium::thread::set_thread_name("_osmium_output");

                std::future<std::string> data_future;
                std::string data;
                do {
                    m_input_queue.wait_and_pop(data_future);
                    data = data_future.get();
                    m_compressor->write(data);
                } while (!data.empty());

                m_compressor->close();
            }

        }; // class OutputThread

        /**
         * This is the user-facing interface for writing OSM files. Instantiate
         * an object of this class with a file name or osmium::io::File object
         * and optionally the data for the header and then call operator() on it
         * to write Buffers. Call close() to finish up.
         */
        class Writer {

            osmium::io::File m_file;

            std::unique_ptr<osmium::io::detail::OutputFormat> m_output;
            osmium::io::detail::data_queue_type m_output_queue {};

            std::unique_ptr<osmium::io::Compressor> m_compressor;

            osmium::thread::CheckedTask<OutputThread> m_output_task;

        public:

            /**
             * The constructor of the Writer object opens a file and writes the
             * header to it.
             *
             * @param file File (contains name and format info) to open.
             * @param header Optional header data. If this is not given sensible
             *               defaults will be used. See the default constructor
             *               of osmium::io::Header for details.
             *
             * @throws std::runtime_error If the file could not be opened.
             * @throws std::system_error If the file could not be opened.
             */
            explicit Writer(const osmium::io::File& file, const osmium::io::Header& header = osmium::io::Header(), bool allow_overwrite=false) :
                m_file(file),
                m_output(osmium::io::detail::OutputFormatFactory::instance().create_output(m_file, m_output_queue)),
                m_compressor(osmium::io::CompressionFactory::instance().create_compressor(file.compression(), osmium::io::detail::open_for_writing(m_file.filename(), allow_overwrite))),
                m_output_task(OutputThread {m_output_queue, m_compressor.get()}) {
                m_output->write_header(header);
            }

            explicit Writer(const std::string& filename, const osmium::io::Header& header = osmium::io::Header(), bool allow_overwrite=false) :
                Writer(osmium::io::File(filename), header, allow_overwrite) {
            }

            explicit Writer(const char* filename, const osmium::io::Header& header = osmium::io::Header(), bool allow_overwrite=false) :
                Writer(osmium::io::File(filename), header, allow_overwrite) {
            }

            Writer(const Writer&) = delete;
            Writer& operator=(const Writer&) = delete;

            ~Writer() {
                close();
            }

            /**
             * Write contents of a buffer to the output file.
             *
             * @throws Some form of std::runtime_error when there is a problem.
             */
            void operator()(osmium::memory::Buffer&& buffer) {
                m_output_task.check_for_exception();
                m_output->write_buffer(std::move(buffer));
            }

            /**
             * Flush writes to output file and close it. If you do not
             * call this, the destructor of Writer will also do the same
             * thing. But because this call might thrown an exception,
             * it is better to call close() explicitly.
             *
             * @throws Some form of std::runtime_error when there is a problem.
             */
            void close() {
                m_output->close();
                m_output_task.close();
            }

        }; // class Writer

    } // namespace io

} // namespace osmium

#endif // OSMIUM_IO_WRITER_HPP
