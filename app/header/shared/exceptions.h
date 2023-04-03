class OutOfSyncException : public std::exception
{
public:
    virtual const char *what() const throw()
    {
        return "Out of sync";
    }
};

//  Currently not in use
class ProjectConfigException : public std::exception
{
public:
    virtual const char *what() const throw()
    {
        return "Project config messages exchange error";
    }
};

class ConnectionLostException : public std::exception
{
public:
    virtual const char *what() const throw()
    {
        return "Connection lost";
    }
};