#include <fstream>
#include <sstream>
#include <string>
#include <iostream>

using std::ostream;
using std::stringstream;
using std::endl;
using std::string;
using std::ofstream;
using std::cout;

class OutputStream {
    stringstream s;
    string filename;
public:
    ostream& stream() {return s;}
    void flush_to_cout() const {
        cout<<s.str();
    }
    void flush_to_file(const string& filename) {
        ofstream f {filename};
        f<<s.str();
    }
}; 

template<typename T>
OutputStream& operator<<(OutputStream& os, T&& t)
{
    os.stream()<<t;    
    return os;
}

OutputStream& operator<< (OutputStream& os,ostream& (*pf)(ostream&)) {
    os.stream()<<pf;
    return os;
}