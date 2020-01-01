// Cytosim was created by Francois Nedelec. Copyright 2007-2017 EMBL.

#ifndef PARSER_H
#define PARSER_H

#include "interface.h"


/// Parser to read and execute Cytosim config files
/**
 This is where the syntax of the config file is defined
 */
class Parser : public Interface
{
private:
    
    /// disabled default constructor
    Parser();
    
    /// control switch to enable command 'set' (creating a property)
    bool      do_set;
    
    /// control switch to enable command 'change' (change a property)
    bool      do_change;
    
    /// control switch to enable command 'new' and 'delete' (object)
    bool      do_new;
    
    /// control switch to enable command 'run' (run simulation)
    bool      do_run;
    
    /// control switch to enable command 'write' (write files)
    bool      do_write;
    
    /// print the lines located between `pos` and current position
    void      show_lines(std::istream&, std::streampos);

public:
    
    /// set the permission of the parser
    Parser(Simul& s, bool can_set, bool can_change, bool can_new, bool can_run, bool can_write);
    
    //-------------------------------------------------------------------------------
    
    /// parse command `set
    void      parse_set(std::istream&);
    
    /// parse command `change`
    void      parse_change(std::istream&);
    
    /// parse command `new`
    void      parse_new(std::istream&);
    
    /// parse command `delete`
    void      parse_delete(std::istream&);
    
    /// parse command `mark`
    void      parse_mark(std::istream&);

    /// parse command `cut`
    void      parse_cut(std::istream&);

    /// parse command `run`
    void      parse_run(std::istream&);
    
    /// parse command `read`
    void      parse_read(std::istream&);
    
    /// parse command `write`
    void      parse_report(std::istream&);
    
    /// parse command `import`
    void      parse_import(std::istream&);
    
    /// parse command `export`
    void      parse_export(std::istream&);
    
    /// parse command `call`
    void      parse_call(std::istream&);
    
    /// parse command `repeat`
    void      parse_repeat(std::istream&);

    /// parse command `for`
    void      parse_for(std::istream&);
    
    /// parse command `end`
    void      parse_end(std::istream&);
    
    //-------------------------------------------------------------------------------

    /// Parse content of stream, recording position if pointer is given
    void      evaluate(std::istream&, std::streampos&);
    
    /// Parse code in string, and report errors
    void      evaluate(std::string const&);

    /// Open and parse the config file with the given name
    int       readConfig(std::string const& name);

    /// Parse the default config file i.e. calls readConfig(simul.prop->config)
    int       readConfig();

};

#endif

