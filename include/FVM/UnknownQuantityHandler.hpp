#ifndef _DREAM_FVM_UNKNOWN_QUANTITY_HANDLER_HPP
#define _DREAM_FVM_UNKNOWN_QUANTITY_HANDLER_HPP

namespace DREAM::FVM { class UnknownQuantityHandler; }

#include <string>
#include <vector>
#include "FVM/config.h"
#include "FVM/Grid/Grid.hpp"
#include "FVM/UnknownQuantity.hpp"

namespace DREAM::FVM {
    class UnknownQuantityHandler {
    private:
        std::vector<UnknownQuantity*> unknowns;

    public:
        UnknownQuantityHandler();
        ~UnknownQuantityHandler();

        UnknownQuantity *GetUnknown(const len_t i) { return unknowns.at(i); }
        len_t GetUnknownID(const std::string&);
        len_t GetNUnknowns() const { return this->unknowns.size(); }
        len_t Size() const { return GetNUnknowns(); }

        real_t *GetUnknownData(const len_t);
        real_t *GetUnknownDataPrevious(const len_t);

        bool HasInitialValue(const len_t id) const { return unknowns[id]->HasInitialValue(); }
        len_t InsertUnknown(const std::string&, Grid*);

        void Store(std::vector<len_t>&, Vec&);
        void Store(const len_t id, Vec& v, const len_t offs) { unknowns[id]->Store(v, offs); }
        void Store(const len_t id, const real_t *v, const len_t offs=0) { unknowns[id]->Store(v, offs); }

        void SaveStep(const real_t t);

        void SaveSFile(const std::string& filename, bool saveMeta=false);
        void SaveSFile(SFile*, const std::string& path="", bool saveMeta=false);

        void SetInitialValue(const std::string&, const real_t*, const real_t t0=0);
        void SetInitialValue(const len_t, const real_t*, const real_t t0=0);

        UnknownQuantity *operator[](const len_t i) { return GetUnknown(i); }
    };
}

#endif/*_DREAM_FVM_UNKNOWN_QUANTITY_HANDLER_HPP*/
