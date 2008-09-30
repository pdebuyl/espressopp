#include "logging.hpp"
#include "mpi.hpp"
#include "boost/python.hpp"
#include "pmi/pmi.hpp"
#include "hello.hpp"

/** called when python exits to clean up. */
static void finalize_espresso();

BOOST_PYTHON_MODULE(_espresso)
{
  logging::initLogging();
  mpi::initMPI();

  Py_AtExit(&finalize_espresso);

  if (!pmi::mainLoop()) {
    // the controller: register with python
    hello::PHelloWorld::registerPython();
  } else {
    /*
      a worker: pmi::mainLoop only exits after
      the controller has disconnected, and we simply
      quit without giving control back to the
      python script.
    */
    Py_Exit(0);
  }
}

void finalize_espresso() {
  if (pmi::isController()) {
    pmi::endWorkers();
  }
  mpi::finalizeMPI();
}