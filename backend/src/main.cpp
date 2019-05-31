#include "crow_all.h"
#include "GPTree.hpp"
#include "gtree_query.hpp"
#include <sstream>
#include <boost/algorithm/string.hpp>
#include <algorithm>
#include <vector>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <cstring>

// class ExampleLogHandler : public crow::ILogHandler {
//     public:
//         void log(std::string /*message*/, crow::LogLevel /*level*/) override {
// //            cerr << "ExampleLogHandler -> " << message;
//         }
// };

// cab should be not more than 10KM away
const int MAX_CAB_DIST = 10000;
// total number of all cabs
int MAX_CAB_NUM = 0;

crow::json::wvalue processRequest(double psg_lng, double psg_lat, double dst_lng, double dst_lat) {
  int S, T;  // node index of the location the passenger & destination
  double min_dist_psg = DBL_MAX;
  double min_dist_dst = DBL_MAX;
  for (auto iter=Nodes.begin(), end=Nodes.end(); iter!=end; ++iter) {
    double dist_lng = iter->x - psg_lng;
    double dist_lat = iter->y - psg_lat;
    double dist = dist_lng*dist_lng + dist_lat*dist_lat;
    if (min_dist_psg > dist) {
      S = iter - Nodes.begin();
      min_dist_psg = dist;
    }
    dist_lng = iter->x - dst_lng;
    dist_lat = iter->y - dst_lat;
    dist = dist_lng*dist_lng + dist_lat*dist_lat;
    if (min_dist_dst > dist) {
      T = iter - Nodes.begin();
      min_dist_dst = dist;
    }
  }
  CROW_LOG_INFO << "snode:" << S << " enode:" << T;
  int D4 = tree.search(S, T);
  // printf("D4=%d\n", D4);

  int knn_size = 100;             // size of knn search
  int max_dist = 0;               // max distance of latest knn search result
  vector<int> avail_cab;          // id of available suitable cabs
  vector<vector<int> > shortPath; // corresponding path
  vector<int> reachLength;        // the distance between psg & dest on the path
  vector<int> cab_psg_num;        // # of psgrs on board
  vector<int> away;

  /* Repeat searching until:
    1. we already found 5 available cabs
    2. we have moved out of searching range (>MAX_CAB_DIST)
    3. we have looked into all cabs we got
  */
  int laststop = 0;   // where the last round search ends, useful when retrying
  int lastnode = -1;  // index of the last node we checked
  while (avail_cab.size() < 5 && max_dist <= MAX_CAB_DIST && knn_size < MAX_CAB_NUM) {
    // clear, enlarge K(NN), retry
    // printf("new search round, knn_size:%d, already found:%d\n", knn_size, avail_cab.size());
    std::vector<ResultSet> cadiCabs = knn_query(S, knn_size);
    max_dist = cadiCabs.back().dis;
    for (auto iter=cadiCabs.begin()+laststop, end=cadiCabs.end(); iter!=end; ++iter) {
      // 0. check for duplicate nodes (more than one cabs at the same node)
      if (lastnode == iter->id) {
        continue;
      }
      lastnode = iter->id;
      // 1. check if the node is within 10 km
      int D2 = iter->dis;
      if (D2 > MAX_CAB_DIST) {
        goto end;
      }
      // iterate all cab(s) at this node
      for (const Cab& cab : Nodes[lastnode].cabs) {
        vector<int> totalpath;
        int reach;
        switch (cab.psg_num) {
          case 0: { // cab got no passenger yet
            vector<int> reachPath;
            D2 = tree.find_path(lastnode, S, totalpath);
            int D3 = tree.find_path(S, T, reachPath);
            reach = D3;
            // printf("reach path size: %d\n", reachPath.size());
            // printf("  [from %d to %d]\n", reachPath.front(), reachPath.size());
            // printf("pickup path size: %d\n", totalpath.size());
            // printf("  [from %d to %d]\n", totalpath.front(), totalpath.size());
            for (int i : reachPath) { // TODO: before combining, clear overlap point
              totalpath.push_back(i);
            }
            // printf("total path size: %d\n", totalpath.size());
            goto push;
            // break;
          }
          case 1:
          case 2:
          case 3: { // cab got passenger(s) on board
            D2 = tree.find_path(lastnode, S, totalpath);
            int psg_num = cab.psg_num;
            int psg_num1 = psg_num + 1;
            int seq1[3];
            int seq2[4];
            int ans[4];
            int psg[4];
            for (int i=0; i!=psg_num; ++i) {
              seq1[i] = i;
              seq2[i] = i;
              psg[i] = cab.psg_node[i];
            }
            seq2[psg_num] = psg_num;
            psg[psg_num] = T;

            // distance matrix among destinations
            int dist[4][4];
            for (int i=0; i!=psg_num; ++i) {
              for (int j=i+1; j!=psg_num1; ++j) {
                dist[i][j] = tree.search(psg[i], psg[j]);
              }
            }
            for (int i=1; i!=psg_num1; ++i) {
              for (int j=0; j!=i; ++j) {
                dist[i][j] = dist[j][i];
              }
            }
            // distances FROM current pos of cab
            // TO the destinations of each psg already on board
            int cur_dist[3];
            for (int i=0; i!=psg_num; ++i) {
              cur_dist[i] = tree.search(psg[i], iter->id);
            }
            // distances FROM current pos of calling psg
            // TO the destinations of all psgs
            int new_dist[4];
            for (int i=0; i!=psg_num1; ++i) {
              new_dist[i] = tree.search(psg[i], S);
            }

            int D1 = INT32_MAX;
            do {
              int total = cur_dist[seq1[0]];
              for (int i=1; i!=psg_num; ++i) {
                total += dist[seq1[i]][seq1[i-1]];
              }
              if (total < D1) {
                D1 = total;
              }
            } while ( std::next_permutation(seq1,seq1+psg_num) );

            int D3_1 = INT32_MAX;
            int D3_2 = INT32_MAX;
            int D3 = INT32_MAX;
            do {
              int _D3_1, _D3_2;
              int _D3 = new_dist[seq2[0]];
              if (seq2[0] == 4) {
                _D3_2 = _D3;
              } else {
                _D3_1 = _D3;
              }
              for (int i=1; i!=psg_num1; ++i) {
                _D3 += dist[seq2[i]][seq2[i-1]];
                if (seq2[i] == psg_num) {
                  _D3_2 = _D3;
                } else {
                  _D3_1 = _D3;
                }
              }
              if (_D3 < D3) {
                D3 = _D3;
                D3_1 = _D3_1;
                D3_2 = _D3_2;
                for (int i=0; i!=psg_num1; ++i) {
                  ans[i] = seq2[i];
                }
              }
            } while ( std::next_permutation(seq2,seq2+psg_num1) );

            // printf("D1:%d D2:%d D3:%d D3_1:%d D3_2:%d D4:%d\n", D1, D2, D3, D3_1, D3_2, D4);
            // printf("D2+D3_1-D1:%d, D3_2-D4:%d\n", D2+D3_1-D1, D3_2-D4);
            reach = D3_2;
            if (D2+D3_1-D1 <= MAX_CAB_DIST && D3_2-D4 <= MAX_CAB_DIST) {
              // printf("[from %d to %d]\n", totalpath.front(), totalpath.back());
              vector<int> tmppath;
              tree.find_path(S, psg[ans[0]], tmppath);
              for (int j : tmppath) {// TODO: before combining, clear overlap point
                totalpath.push_back(j);
              }
              for (int i=1; i!=psg_num1; ++i) {
                tree.find_path(psg[ans[i-1]], psg[ans[i]], tmppath);
                // printf("[from %d to %d]\n", tmppath.front(), tmppath.back());
                for (int j : tmppath) {// TODO: before combining, clear overlap point
                  totalpath.push_back(j);
                }
              }
              goto push;
            }
            break;
          }
          default:
            // cab full, no available seat
            break;
        }
        continue;
      push:
        avail_cab.push_back(cab.id);
        shortPath.push_back(totalpath);
        reachLength.push_back(reach);
        cab_psg_num.push_back(cab.psg_num);
        away.push_back(D2);
        if (avail_cab.size() >= 5) {
          goto end;
        }
      }
    }
  end:
    laststop = knn_size;
    knn_size *= 10;
  }
  // for (const vector<int>& i : shortPath) {
  //   printf("pathsize:%lu\n", i.size());
  //   for (int j : i) {
  //     printf("pathNode:%d\n", j);
  //   }
  // }
  // for (const vector<int>& i : shortPath) {
  //   printf("pathsize:%lu\n", i.size());
  // }
  // for (int i : avail_cab) {
  //   printf("cab:%d\n", i);
  // }

  CROW_LOG_INFO << "found " << avail_cab.size() << " cab(s)";
  crow::json::wvalue ans;
  // ans["cab"] = avail_cab;
  // ans["len"] = reachLength;
  for (int i=0; i!=shortPath.size(); ++i) {
    for (int j=0; j!=shortPath[i].size(); ++j) {
      ans["cab"][i]["path"][j]["lng"] = Nodes[shortPath[i][j]].x;
      ans["cab"][i]["path"][j]["lat"] = Nodes[shortPath[i][j]].y;
    }
    ans["cab"][i]["id"] = avail_cab[i];
    ans["cab"][i]["len"] = reachLength[i];
    ans["cab"][i]["psg_num"] = cab_psg_num[i];
    ans["cab"][i]["away"] = away[i];
  }
  ans["T"]["lng"] = Nodes[T].x;
  ans["T"]["lat"] = Nodes[T].y;
  ans["S"]["lng"] = Nodes[S].x;
  ans["S"]["lat"] = Nodes[S].y;
  return ans;
}

int main() {
  /*
    knn query initiation
  */
  init();
  gtree_load();
	hierarchy_shortest_path_load();
	pre_query();
  /*
    read & store cab data
    node data stored in (vector<struct Node> Nodes) by gtree
  */
  ifstream fin("../data/car.txt");
  std::string buffer;
  std::vector<std::string> cabinfo;
  std::vector<std::string> posinfo;
  MAX_CAB_NUM = 0;
  while (getline(fin, buffer)) {
    boost::split(cabinfo, buffer, boost::is_any_of(" "), boost::token_compress_on);
    Cab newcab;
    newcab.id = MAX_CAB_NUM;
    newcab.psg_num = atoi(cabinfo[1].c_str());
    // printf("newcab.psg_num:%d\n", newcab.psg_num);
    boost::split(posinfo, cabinfo[2], boost::is_any_of(","), boost::token_compress_on);
    int node = atoi(posinfo[2].c_str());
    for (int i=0; i!=newcab.psg_num; ++i) {
      boost::split(posinfo, cabinfo[i+3], boost::is_any_of(","), boost::token_compress_on);
      // printf("%lu\n", posinfo.size());
      newcab.psg_node[i] = atof(posinfo[2].c_str());
    }
    Nodes[node].cabs.push_back(newcab);
    ++MAX_CAB_NUM;
  }
  fin.close();
  printf("MAX_CAB_NUM:%d\n", MAX_CAB_NUM);

  /*
    GPTree initiation
  */
  srand(747929791);
  read();
	Additional_Memory=2*G.n*log2(G.n);
  load();


  crow::SimpleApp app;

  CROW_ROUTE(app,"/hello/<int>")
  ([](int count){
      if (count > 100)
          return crow::response(400);
      std::ostringstream os;
      os << count << " bottles of beer!";
      return crow::response(os.str());
  });

  CROW_ROUTE(app, "/api/add_json")
    .methods(crow::HTTPMethod::POST)
  ([](const crow::request& req){
      auto x = crow::json::load(req.body);
      CROW_LOG_INFO << "msg from client: " << req.body;
      if (x) {
        crow::json::wvalue reply = processRequest(
          x["psg_lng"].d(),
          x["psg_lat"].d(),
          x["dst_lng"].d(),
          x["dst_lat"].d());
        return crow::response{reply};
      }
      return crow::response(400);
  });

  // ignore all log
  crow::logger::setLogLevel(crow::LogLevel::Debug);
  //crow::logger::setHandler(std::make_shared<ExampleLogHandler>());

  app.port(18080)
      .multithreaded()
      .run();
}
