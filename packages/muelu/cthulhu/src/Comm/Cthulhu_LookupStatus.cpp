#include "Cthulhu_LookupStatus.hpp"
#include "Cthulhu_Exceptions.hpp"

namespace Cthulhu {

#ifdef HAVE_CTHULHU_TPETRA

  const Cthulhu::LookupStatus toCthulhu(Tpetra::LookupStatus LS) {
    
    if (LS == Tpetra::AllIDsPresent)
      return Cthulhu::AllIDsPresent;
    if (LS == Tpetra::IDNotPresent)
      return Cthulhu::IDNotPresent;
    
    TEST_FOR_EXCEPTION(1, Cthulhu::Exceptions::RuntimeError, "Unknown LookupStatus"); 
    
  }

  const Tpetra::ProfileType toTpetra(Cthulhu::ProfileType PT) {

    if (PT == Cthulhu::StaticProfile)
      return Tpetra::StaticProfile;
    if (PT == Cthulhu::DynamicProfile)
      return Tpetra::DynamicProfile;
    
    TEST_FOR_EXCEPTION(1, Cthulhu::Exceptions::RuntimeError, "Unknown ProfileType"); 

  }

  const Tpetra::OptimizeOption toTpetra(Cthulhu::OptimizeOption os) {

    if (os == Cthulhu::DoOptimizeStorage)
      return Tpetra::DoOptimizeStorage;
    if (os == Cthulhu::DoNotOptimizeStorage)
      return Tpetra::DoNotOptimizeStorage;
    
    TEST_FOR_EXCEPTION(1, Cthulhu::Exceptions::RuntimeError, "Unknown OptimizeOption"); 

  }

#endif // HAVE_CTHULHU_TPETRA

#ifdef HAVE_CTHULHU_EPETRA

  const Cthulhu::LookupStatus toCthulhu(int) {
    return Cthulhu::AllIDsPresent; // TODO: manage error of EpetraMap RemoteIDList (return -1) + return the correct LookupStatus
  }

  const bool toEpetra(Cthulhu::ProfileType PT) {
    
    if (PT == Cthulhu::StaticProfile)
      return true;
    if (PT == Cthulhu::DynamicProfile)
      return false;
    
    TEST_FOR_EXCEPTION(1, Cthulhu::Exceptions::RuntimeError, "Unknown ProfileType"); 
  }

  const bool toEpetra(Cthulhu::OptimizeOption os) {

    if (os == Cthulhu::DoOptimizeStorage)
      return true;
    if (os == Cthulhu::DoNotOptimizeStorage)
      return false;
    
    TEST_FOR_EXCEPTION(1, Cthulhu::Exceptions::RuntimeError, "Unknown OptimizeOption"); 

  }

#endif // HAVE_CTHULHU_EPETRA

} // namespace cthulhu

