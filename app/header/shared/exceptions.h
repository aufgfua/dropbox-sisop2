class OutOfSyncException : public std::exception
{
public:
    virtual const char *what() const throw()
    {
        return "Out of sync";
    }
};

class ProjectConfigException : public std::exception
{
public:
    virtual const char *what() const throw()
    {
        return "Project config messages exchange error";
    }
};