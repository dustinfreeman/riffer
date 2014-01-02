#include <map>
#include <string>
#include <iostream>

namespace rfr {

	namespace tags {
		//should not access _tags directly.
		std::map<std::string, std::string> _tags;

		void register_tag(std::string _name, std::string _tag) {
			bool already_registered = false;
			//check if name already registered
			if (_tags.find(_name) == _tags.end()) {
				std::cout << "tag name already registered: " << _name << "\n";
				already_registered = true;
			}
			//check if tag already registered
			std::map<std::string,std::string>::iterator it;
			for (it = _tags.begin(); it != _tags.end(); it++) {
				//apparently it->second is how to access the key.
				if (it->second == _tag) {
					std::cout << "tag name already registered: " << _name << "\n";
					already_registered = true;
				}	
			}
			if (already_registered)
				return;

			_tags[_name] = _tag;
		}

		void register_from_file(std::string filename) {
			//will iteratively call register based on lines of a given file.
			//Expected file format:
			//tag name 1 = tag key 1
			//tag name 2 = tag key 2

			//TODO register_from_file
			std::cout << "register_from_file not implemented \n";
		}

		//this function will be called often. Cannot have high performance cost.
		std::string get_tag(std::string _name) {
			//WARNING: no error checking.
			return _tags.find(_name)->second;
		}
		//We have decided to do the above at run-time, 
		//as opposed to (likely faster) compile-time with tag key constants
		//as this is easier for inheritance.
	};

};
