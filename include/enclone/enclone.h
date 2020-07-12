#ifndef ENCLONE_H
#define ENCLONE_H

#include <iostream>
#include <boost/asio.hpp> // unix domain local sockets

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

class enclone{
    private:

        
    public:
        enclone();
        ~enclone();

};

#else // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
# error Local sockets not available on this platform.
#endif // defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)

#endif