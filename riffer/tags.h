#include <map>
#include <string>
#include <string>
#include <iostream>

namespace rfr {

#define TAG_SIZE 4
#define RIFF_SIZE sizeof(int) //expected: 4
//RIFF_SIZE is the number of bytes to define the chunk size.

#define UNDEFN_TYPE -1
#define INT_TYPE 0
#define LONG_TYPE 1
#define CHAR_PTR_TYPE 3

#define NULL_TAG "NULL"

	namespace tags {
		struct tag_defn {
			std::string name;
			std::string tag;
			int type_id;
			tag_defn(std::string _name = NULL_TAG, std::string _tag = NULL_TAG, int _type_id = UNDEFN_TYPE)
				: name(_name), tag(_tag), type_id(_type_id) 
			{}
		};

		//should not access _tags directly externally.
		std::map<std::string, tag_defn> _tags;

		void register_tag(std::string _name, std::string _tag, int _type_id) {
			bool already_registered = false;
			//check if name already registered
			if (_tags.find(_name) != _tags.end()) {
				std::cout << "tag name already registered: " << _name << "\n";
				already_registered = true;
			}
			//check if tag already registered
			std::map<std::string,tag_defn>::iterator it;
			for (it = _tags.begin(); it != _tags.end(); it++) {
				//apparently it->second is how to access the key.
				if (it->second.tag == _tag) {
					std::cout << "tag name already registered: " << _name << "\n";
					already_registered = true;
				}	
			}
			if (already_registered)
				return;

			_tags[_name] = tag_defn(_name, _tag, _type_id);
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
			return _tags.find(_name)->second.tag;
		}
		//We have decided to do the above at run-time, 
		//as opposed to (likely faster) compile-time with tag key constants
		//as this is easier for inheritance.

		int get_type_id_from_tag(std::string _tag) {
			std::map<std::string,tag_defn>::iterator it;
			for (it = _tags.begin(); it != _tags.end(); it++) {
				if (_tag == it->second.tag)
					return it->second.type_id;
			}
			return UNDEFN_TYPE;
		}
	};
};
