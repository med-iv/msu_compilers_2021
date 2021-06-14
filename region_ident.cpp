#ifdef __cplusplus
#define export exports
extern "C" {
    #include <qbe/all.h>
}
#undef export
#else
#include "qbe/all.h"
#endif

#include <iostream>
#include <utility>
#include <unordered_map>
#include <set>
#include <vector>
#include <algorithm>
#include <cstring>
using namespace std;

//std::set<Blk*> get_cycle(Fn *fn, int id_from, int id_to);
void readfn(Fn *fn);

struct Region {
    unsigned int id;
    string name;
    std::set<int> preds;
//    std::vector<Region*> pasts;
};

// чтобы отладить
void print_cycle(std::set<Blk*> cycle) {
    for (auto it = cycle.begin(); it != cycle.end(); ++it) {
        cout << (*it)->name << " ";
    }
    cout << endl;
}


void print_region_cycle(std::set<int> cycle, unordered_map<int, Region>& regions) {
    for (auto it = cycle.begin(); it != cycle.end(); ++it) {
        cout <<  regions[(*it)].name << " ";
    }
    cout << endl;
}

// помечаем все блоки цикла
void dfs(int id_from, int id_to, std::unordered_map<int, int>& blks_visit, std::unordered_map<int, Blk*>& blks) {
//    cout << id_from << ' ' << id_to << endl;
    if (id_from == id_to or blks[id_from]->npred == 0 or blks_visit[id_from]==1) {
        return;
    } else {
        blks_visit[id_from] = 1;
        Blk *blk = blks[id_from];
        for(int i = 0; i < blk->npred; i++) {
            if ((blk->pred[i])->id > id_to) {
//                cout << "gg" << " " << id_from << ' ' << (blk->pred[i])->id << endl;
                dfs((blk->pred[i])->id, id_to, blks_visit, blks);
            }
        }
    }
}

// получаем по ребру естественный цикл
std::set<Blk*> get_cycle(Fn *fn, int id_from, int id_to, std::unordered_map<int, Blk*>& blks) {
    set<Blk*> cycle = {};

    // visit
    std::unordered_map<int, int> blks_visit = {};
    for(int i=0; i<fn->nblk; i++) {
        blks_visit[i] = 0;
    }
    blks_visit[id_to] = 1;

//    cout << "call" << " " << id_from << ' ' << id_to << endl;
    dfs(id_from, id_to, blks_visit, blks);

    for(auto it = blks_visit.begin(); it != blks_visit.end(); ++it) {
        if ((it->second) == 1) {
            cycle.insert(blks[it->first]);
        }
    }

    return cycle;
}

// не нужна
static void readdat (Dat *dat) {
    std::cout << "dat" << std::endl;
    (void) dat;
}


void readfn(Fn *fn) {
    fillrpo(fn);
    fillpreds(fn);

    // hashmap id в rpo блоков
    std::unordered_map<int, int> blk_rpos = {};

    // id в блок
    std::unordered_map<int, Blk*> blks = {};

    for(int i=0; i<fn->nblk; i++) {
        blk_rpos[fn->rpo[i]->id] = i;
        blks[fn->rpo[i]->id] = fn->rpo[i];

    }


    // список естественных циклов
    std::set<std::set<Blk*>> natural_cycles_1 = {};
    // обратные ребра
    std::vector<std::pair<int, int>> edges = {};


    // идем по всем базовым блокам
    for (Blk *blk = fn->start; blk; blk=blk->link) {
        // идем по предыдущим блокам
        for(int i = 0; i < blk->npred; i++) {
            // если у предыдущего блока номер больше или равен в reverse post order, то значит, это петля цикла
            if (blk_rpos[blk->id] <= blk_rpos[(blk->pred[i])->id]) {
//                cout << "from" << " " << blk->pred[i]->name << " " << blk_rpos[(blk->pred[i])->id] << endl;
//                cout << "to" << " " << blk->name << " " << blk_rpos[blk->id] << endl;
                std::set<Blk*> cycle = get_cycle(fn, (blk->pred[i])->id, blk->id, blks);
//                cout << "blk ids " << (blk->pred[i])->name << " " << blk->name << endl;
//                print_cycle(cycle);
                natural_cycles_1.insert(cycle);
                edges.push_back(make_pair((blk->pred[i])->id, blk->id));
            }
        }
    }
    std::vector<std::set<Blk*>> natural_cycles(natural_cycles_1.begin(), natural_cycles_1.end());
//    cout << natural_cycles.size() << endl;

    // упорядочиваем внутренние циклы
    std::sort(natural_cycles.begin(), natural_cycles.end(), [](std::set<Blk*> a, std::set<Blk*> b) {
        for (auto elem : a) {
            if (b.count(elem) == 0) {
                return false;
            }
        }
        return true;
    });


    // начинаем выделять области
    std::unordered_map<int, Region> regions = {};

    for(int i = 0; i < fn->nblk; i++) {
        Blk *blk = blks[i];
        cout << "@" << blk->name << " : " << ":" << endl;
        strcpy(blk->name, (string(blk->name)).c_str());

        std::set<int> preds = {};
        for(int i = 0; i < blk->npred; i++) {
            preds.insert((blk->pred[i])->id);
        }
        Region region = {blk->id, "@" + string(blk->name) + "__r", preds};
//        Region region = {blk->id, "@" + string(blk->name) + "__r"};
        regions[blk->id] = region;
    }


    std::vector<std::set<int>> regions_natural_cycles = {};
    for(auto it = natural_cycles.begin(); it != natural_cycles.end(); ++it) {
        std::set<Blk*> cycle = *it;
        std::set<int> region_cycle = {};
        for(auto cit = cycle.begin(); cit != cycle.end(); ++cit){
            region_cycle.insert((*cit)->id);
        }
        regions_natural_cycles.push_back(region_cycle);
    }
    std::reverse(regions_natural_cycles.begin(), regions_natural_cycles.end());


    // regions -  хеш-мапа из областей
    // regions_natural_cycles - вектор из множеств id областей, которые в цикле

    /*
     *  Выводим области листы
     *
     *  Пока очередь не пуста:
     *      1. Вытаскиваем цикл из очереди циклов
     *      2. Создаем область из цикла (новый id, имя), выводим область
     *      3. Убираем из regions все области, которые войдут в новую область
     *      4  Убираем из regions_natural_cycles во всех циклах области из цикла на новую область
     *      5  Убираем во всех областях удаленные области
     *      6. Добавляем в regions новую область
     *
     *  Выводим финальную область
     */

    while (regions_natural_cycles.size() > 0) {
        // вытаскиваем цикл
        std::set<int> region_cycle = regions_natural_cycles.back();
        regions_natural_cycles.pop_back();

        unsigned int new_id = (*region_cycle.begin());
        std::set<int> new_preds = {};
        string new_name = "";
        if (region_cycle.size() > 1) {
            new_name = regions[new_id].name + "__r__r";
            // выводим начальную область
            cout << regions[new_id].name << " : ";

            // выводим блоки
            for (auto it = ++region_cycle.begin(); it != region_cycle.end(); ++it) {
                cout << regions[*it].name << " ";
            }
            cout << ":";

            unsigned int post_id_from = 0, post_id_to = 0;

            // выводим ребра
            for (auto it = region_cycle.begin(); it != region_cycle.end(); ++it) {
                Region region = regions[*it];
//                cout << endl << "region name " << region.name;
                for (auto pred_it = region.preds.begin(); pred_it != region.preds.end(); ++pred_it) {
//                    cout << endl << "pred " << regions[*pred_it].name << endl;
                    if (regions[*pred_it].id < region.id) {
                        if (region_cycle.count(*pred_it) > 0) {
                            cout << " " << regions[*pred_it].name << "-" << region.name;
                        } else {
                            new_preds.insert(*pred_it);
                        }
                    }
                }
            }

            cout << endl;

            // Выводим петлю
            cout << regions[new_id].name << "__r : : " << regions[new_id].name << "__r-" << \
                regions[new_id].name << "__r" << endl;
        } else {
            new_name = regions[new_id].name + "__r";
            cout << regions[new_id].name << " : : " << regions[new_id].name << "-" << regions[new_id].name << endl;

            Region region = regions[new_id];
            for (auto pred_it = region.preds.begin(); pred_it != region.preds.end(); ++pred_it) {
                if (regions[*pred_it].id < region.id) {
                    new_preds.insert(*pred_it);
                }
            }
        }

        // Новая область
        Region new_region = {new_id, new_name, new_preds};


        // Убираем из regions_natural_cycles во всех циклах области из цикла на новую область
        for (auto tmp_it = regions_natural_cycles.begin(); tmp_it != regions_natural_cycles.end(); ++tmp_it) {
//            std::set<int> tmp_region_cycle = *tmp_it;
            for (auto it = region_cycle.begin(); it != region_cycle.end(); ++it) {
                if ((*tmp_it).count(*it) > 0) {
                    (*tmp_it).erase(*it);
                    (*tmp_it).insert(new_id);
                }
            }
        }

        // Убираем во всех областях удаленные области и добавляем новую
        for (auto kv: regions) {
            for (auto it = region_cycle.begin(); it != region_cycle.end(); ++it) {
                if (kv.second.preds.count(*it) > 0) {
                    regions[kv.first].preds.erase(*it);
                    regions[kv.first].preds.insert(new_id);
                }
            }
        }

        // Убираем из regions все области, которые войдут в новую область
        for (auto it = region_cycle.begin(); it != region_cycle.end(); ++it) {
            regions.erase(*it);
        }

        // Добавляем новую область
        regions[new_id] = new_region;


//        for (auto it = regions_natural_cycles.begin(); it != regions_natural_cycles.end(); ++it) {
//            cout << endl << "cycles ";
//            cout << (*it).size() << " ";
//            print_region_cycle(*it, regions);
//            cout << endl;
//        }
    }

    // финальная область
    std::vector<int> region_keys = {};
    for(auto kv : regions) {
        region_keys.push_back(kv.first);
    }
    std::sort(region_keys.begin(), region_keys.end());

    cout << regions[region_keys[0]].name << " : ";

    for(auto it = ++(region_keys.begin()); it != region_keys.end(); ++it)  {
        cout << regions[*it].name << " ";
    }

    cout << ":";

    for(auto it = region_keys.begin(); it != region_keys.end(); ++it)  {
        Region region = regions[*it];
        for (auto pred_it = region.preds.begin(); pred_it != region.preds.end(); ++pred_it) {
            cout << " " << regions[*pred_it].name << "-" << region.name;
        }
    }

    cout << endl << endl;

}


int main(void)
{
    parse(stdin, (char *)"<stdin>", readdat, readfn);
    freeall();
    return 0;
}