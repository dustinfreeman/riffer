#include <string>
#include <fstream>

#include "chunk.h"

namespace rfr {
	struct CaptureSession {
		std::fstream capture_file;
		std::string filename;
		CaptureSession(std::string _filename = "capture.dat", bool overwrite = true) {
			filename = _filename;

			std::openmode mode = std::fstream::bin | std::fstream::in | std::fstream::out;
			if (overwrite)
				mode |= std::fstream::trunc //dicard file contents
			else
				mode |= std::fstream::app | std::fstream::ate;
				//lol syntax

			capture_file.open(filename, mode);
		}

		void index_by(std::string tag_name) {
			//informs CaptureSession to index by the given tag.

			//TODO index_by
		}

		void run_index() {
			//clears and re-does any indexing by tags it is supposed to index.
			//will take some time for larger files.

			//TODO run_index
		}

		void add(Chunk chunk) {
			//TODO add to capture_file

			//TODO add to index if chunk contains an indexing parameter.

		}

		void close() {
			//TODO make file available.

		}
	};

};
