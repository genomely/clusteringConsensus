#include <iostream>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <math.h>
#include <algorithm>

using namespace std;

int rowcolToIndex(int numcol, int row, int col){
  return(row*numcol+col);
}

int main (int argc, const char** argv) {

  /* argv[1] = "monoNS5ACon1.2450129-0010.dccs.512.msa"
     argv[2] = numrow
     argv[3] = numcol
     argv[4] = threshold (Minimum number of characters in column to keep it.)
     argv[5] = entropy threshold
     argv[6] = max insert size in alignment. from cmph5ToMSAMaxInserts.py --maxInsert=4
   */

  int length;
  char *buffer;

  ifstream is;
  is.open (argv[1], ios::binary );

  // get length of file:
  is.seekg (0, ios::end);
  length = is.tellg();
  is.seekg (0, ios::beg);

  // allocate memory:
  buffer = new char [length];

  // read data as a block: TODO mmap it
  is.read (buffer,length);
  is.close();

  int numrow = atoi(argv[2]);
  int numcol = atoi(argv[3])+1;
  int maxInsert = atoi(argv[6]);

  cerr << argv[1] << endl;
  cerr << "length= " << length << " should be " << numrow << "*" << numcol << "=" << (numrow*numcol) << endl;
  if (length != (numrow*numcol)){
    exit(1);
  }
  //cout.write (buffer,length);

  ////////////////////////////////
  if (0){
  // Now go through the alignment counting characters
  int numbases;
  char myc;
  for (int cc=0; cc<(numcol-1); cc++){
    numbases = 0;
    for (int rr=0; rr<numrow; rr++){
      myc = buffer[rowcolToIndex(numcol,rr,cc)];
      if ( ((int)myc >= 65) && ((int)myc <= 122) ){
	numbases ++;
      }
    }
    cout << cc << "\t" << numbases << endl;
  }
  }

  ////////////////////////////////
  // How many columns contain the minimum number of characters?
  int threshold = atoi(argv[4]);
  float ethreshold = atof(argv[5]);

  int numbases;
  char myc;
  int numAboveThreshold = 0;

  //// hash
  // unordered_map<int,int> goodColumns;
  //     goodColumns[cc] = 1;
  // for (unordered_map<int,int>::iterator it = goodColumns.begin(); it != goodColumns.end(); it++){
  //   cout << it->first << " " << it->second << endl;
  // }
  // cout << goodColumns[15034] << endl; // 1
  // cout << goodColumns[4] << endl;     // 0

  vector<int> goodColumns;
  double counts[5]; // ACGT-
  double freqs[5]; // ACGT-
  double sumCounts;
  double entropy;

  int doMatchOnly = 0; // Discard all insert columns in the alignment and only look at match.

  for (int cc=0; cc<(numcol-1); cc++){
    if ( (doMatchOnly ==1) && ( (cc % (maxInsert+1)) != 0) ){ continue; } // Skip all non-match positions

    numbases = 0;
    for (int ii=0; ii<5; ii++){ counts[ii]=0.01;}

    for (int rr=0; rr<numrow; rr++){
      myc = buffer[rowcolToIndex(numcol,rr,cc)];

      if (myc=='A'||myc=='a'){ counts[0] += 1.0; }
      if (myc=='C'||myc=='c'){ counts[1] += 1.0; }
      if (myc=='G'||myc=='g'){ counts[2] += 1.0; }
      if (myc=='T'||myc=='t'){ counts[3] += 1.0; }
      if (myc=='-'||myc=='.'){ counts[4] += 1.0; }

      if ( ((int)myc >= 65) && ((int)myc <= 122) ){
	numbases ++;
      }
    }

    // compute entropy
    sumCounts=0.0;
    int tot=5; // 4 for bases and 5 to include delete in entropy

    for (int ii=0; ii<tot; ii++){ sumCounts += counts[ii];}
    for (int ii=0; ii<tot; ii++){ freqs[ii] = counts[ii]/sumCounts;}
    entropy = 0.0;
    for (int ii=0; ii<tot; ii++){ entropy += -freqs[ii]*log(freqs[ii])/log(2.0);}

    // if (cc==3964){
    //   cerr << entropy << "::" << numbases << "::" << counts[0] << "," << counts[1] << "," << counts[2] << "," << counts[3] << "," << counts[4] << endl;
    // }

    // sort to get minor frequency 
    int elements = sizeof(freqs)/sizeof(freqs[0]);
    sort(freqs, freqs+elements);
      
    if (numbases>threshold){
      if (entropy>ethreshold){      
      // if (freqs[3] > 0.08){ // 2nd minor frequency > XX%, rather than entropy
	numAboveThreshold++;
	cerr << "cc " << cc << " numbases " << numbases << " entropy " << entropy << " freqs[4] " << freqs[4] << endl;
	goodColumns.push_back(cc);
      }
    }
  }

  ////////
  // remove all columns that are homopolymer positions in the
  // reference (row==0), so cut down on HP errors affecting the
  // distances.

  unordered_map<int,int> HPCol;
  //     goodColumns[cc] = 1;
  // for (unordered_map<int,int>::iterator it = goodColumns.begin(); it != goodColumns.end(); it++){
  //   cout << it->first << " " << it->second << endl;
  // }
  // goodColumns.count(x)

  char prev,curr,next;
  int numHP = 0;
  for (int pp = 1; pp < (numcol-1); pp++){
    prev = buffer[rowcolToIndex(numcol,0,pp-1)];
    curr = buffer[rowcolToIndex(numcol,0,pp)];
    next = buffer[rowcolToIndex(numcol,0,pp+1)];

    if ((curr == prev) || (curr==next)){
      HPCol[pp] = 1;
      numHP += 1;
    }
  }

  vector<int> filteredColumns;
  for (int i =0; i < goodColumns.size(); i++){
    //    if (HPCol.count(goodColumns[i]) ==0){
    // Take all columns not just non-HP columns now that deletes count less
    if (1){
      filteredColumns.push_back(goodColumns[i]);
    }
  }

  cerr << "at ethreshold=\t" << ethreshold << "\ttreshold=\t" << threshold << "\tnumAboveThreshold=\t" << numAboveThreshold;
  cerr << "\tnumHP=\t" << numHP << "\tfilteredColumns.size()=\t"  << filteredColumns.size() << endl;

  cerr << "filtered columns ";
  for (int i =0; i < filteredColumns.size(); i++){
    cerr << filteredColumns[i] << " ";
  }
  cerr << endl;

  ////////////////////////////////
  // for all pairs of sequences, compute distances on all filteredColumns
  // All delete mismatches  count as matches.
  unordered_map<string, double> dist;  // "<from><to>"
  dist["AA"] = 0.0;
  dist["CA"] = 1.0;
  dist["GA"] = 1.0;
  dist["TA"] = 1.0;
  dist["-A"] = 1.0;
  dist[" A"] = 0.1;
  
  dist["AC"] = 1.0;
  dist["CC"] = 0.0;
  dist["GC"] = 1.0;
  dist["TC"] = 1.0;
  dist["-C"] = 1.0;
  dist[" C"] = 0.1;
  
  dist["AG"] = 1.0;
  dist["CG"] = 1.0;
  dist["GG"] = 0.0;
  dist["TG"] = 1.0;
  dist["-G"] = 1.0;
  dist[" G"] = 0.1;
  
  dist["AT"] = 1.0;
  dist["CT"] = 1.0;
  dist["GT"] = 1.0;
  dist["TT"] = 0.0;
  dist["-T"] = 1.0;
  dist[" T"] = 0.1;
  
  dist["aa"] = 0.0;
  dist["ca"] = 1.0;
  dist["ga"] = 1.0;
  dist["ta"] = 1.0;
  dist[".a"] = 1.0;
  dist[" a"] = 0.1;
  
  dist["ac"] = 1.0;
  dist["cc"] = 0.0;
  dist["gc"] = 1.0;
  dist["tc"] = 1.0;
  dist[".c"] = 1.0;
  dist[" c"] = 0.1;
  
  dist["ag"] = 1.0;
  dist["cg"] = 1.0;
  dist["gg"] = 0.0;
  dist["tg"] = 1.0;
  dist[".g"] = 1.0;
  dist[" g"] = 0.1;
  
  dist["at"] = 1.0;
  dist["ct"] = 1.0;
  dist["gt"] = 1.0;
  dist["tt"] = 0.0;
  dist[".t"] = 1.0;
  dist[" t"] = 0.1;
  
  dist["A-"] = 1.0;
  dist["C-"] = 1.0;
  dist["G-"] = 1.0;
  dist["T-"] = 1.0;
  dist["--"] = 0.0;
  dist[" -"] = 0.1;
  
  dist["a."] = 1.0;
  dist["c."] = 1.0;
  dist["g."] = 1.0;
  dist["t."] = 1.0;
  dist[".."] = 0.0;
  dist[" ."] = 0.1;
  
  dist["A "] = 0.1;
  dist["C "] = 0.1;
  dist["G "] = 0.1;
  dist["T "] = 0.1;
  dist["a "] = 0.1;
  dist["c "] = 0.1;
  dist["g "] = 0.1;
  dist["t "] = 0.1;
  dist["- "] = 0.1;
  dist[". "] = 0.1;
  dist["  "] = 0.1;

  int thisc;
  double dsum;
  string key;
  double ratio;
  for (int ii=0; ii<(numrow-1); ii++){
    for (int jj=(ii+1); jj<numrow; jj++){
      dsum = 0.0;
      for (int cc=0; cc<filteredColumns.size(); cc++){
	thisc = filteredColumns[cc];
	key = "";
	key += buffer[rowcolToIndex(numcol,ii,thisc)];
	key += buffer[rowcolToIndex(numcol,jj,thisc)];
	// cout << thisc << "\t" << key << "\t" << dist[key] << endl;
	dsum += dist[key];
      }
      ratio = dsum/(double)filteredColumns.size();
      cout << ii << "\t" << jj << "\t" << ratio << endl;
    }
  }

  delete[] buffer;
  return 0;
}