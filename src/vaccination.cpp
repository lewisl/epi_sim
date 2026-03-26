#include "lib_includes.h"
#include "vaccination.h"

using spreadfunc = std::function<float(std::pair<int,int>, float, std::vector<float>, std::string)>;

/*
    genvaxspreadfunc(dayrange, targetpct, pattern; shotmode=:all)

Create a function that implements the vaccination schedule for a given vaccine type.
This function, when called with a simulation day, returns the percentage of the
target population to receive the vaccine on that day.
*/
// spreadfunc genvaxspreadfunc(std::pair<int, int> dayrange, float targetpct, std::vector<float> pattern, std::string shotmode) {
//   size_t schedlength = dayrange.second - dayrange.first + 1uz;
//   distribscale = pattern .* ((length(pattern)-1)/schedlength)
//   interp = LinearInterpolation(0:length(distribscale)-1, distribscale)
//   startday = dayrange.start; endday1 = dayrange.stop

//   function vaxpctperday(day)
//       p = (day - startday + 1) * round((length(interp)-1) / schedlength, digits=4)
//       p = p < length(pattern) - 1 ? p : length(pattern) - 1 
//       return interp(p) * targetpct
//   end
// }