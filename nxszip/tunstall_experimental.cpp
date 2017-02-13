#ifdef EXPERIMENTAL

#include <assert.h>
#include <math.h>
#include <string.h>
#include <deque>
#include <algorithm>
#include <iostream>
#include "tunstall_experimental.h"

using namespace std;

struct TSymbol {
    int offset;
    int length;
    uint32_t probability;
};

int Tunstall::compress(CStream &stream, unsigned char *data, int size) {

    getProbabilities(data, size);

    //createDecodingTables();
    createEncodingTables();

    int compressed_size;
    unsigned char *compressed_data = compress(data, size, compressed_size);

    stream.write<uchar>(probabilities.size());
    stream.writeArray<uchar>(probabilities.size()*2, (uchar *)&*probabilities.begin());


    stream.write<int>(size);
    stream.write<int>(compressed_size);
    stream.writeArray<unsigned char>(compressed_size, compressed_data);
    delete []compressed_data;
    return compressed_size;
    return 1 + probabilities.size()*2 + 4 + 4 + compressed_size;
}

void Tunstall::decompress(CStream &stream, std::vector<unsigned char> &data) {

    int nsymbols = stream.read<uchar>();
    uchar *probs = stream.readArray<uchar>(nsymbols*2);
    probabilities.resize(nsymbols);
    memcpy(&*probabilities.begin(), probs, nsymbols*2);

    createDecodingTables();

    int size = stream.read<int>();
    data.resize(size);
    int compressed_size = stream.read<int>();
    unsigned char *compressed_data = stream.readArray<unsigned char>(compressed_size);
    if(size)
        decompress(compressed_data, compressed_size, &*data.begin(), size);
}


void Tunstall::getProbabilities(unsigned char *data, int size) {

#ifdef DEBUG_ENTROPY
    double e = 0;
#endif
    probabilities.clear();

    std::vector<int> probs(256, 0);
    for(int i = 0; i < size; i++)
        probs[data[i]]++;
    int max = 0;
    for(int i = 0; i < probs.size(); i++) {
        if(probs[i] > 0) {
            max = i;
#ifdef DEBUG_ENTROPY
            double p = probs[i]/(double)size;
            e -= p * log2(p);
            cout << (char)(i + 65) << " P: " << p << endl;
#endif
            probabilities.push_back(Symbol(i, probs[i]*255/size));
        }
    }
#ifdef DEBUG_ENTROPY
    cout << "Entropy: " << e << " theorical compression: " << e*size/8 << endl;
#endif
}

void Tunstall::setProbabilities(float *probs, int n_symbols) {
    probabilities.clear();
    for(int i = 0; i < n_symbols; i++) {
        if(probs[i] <= 0) continue;
        probabilities.push_back(Symbol(i, probs[i]*255));
    }
}

struct Entry {
    int id;
    unsigned int prob;
    Entry(int i = 0, int p = 0): id(i), prob(p) {}
    bool operator<(const Entry &e) const { return prob < e.prob; }
};

void Tunstall::append(int old_index, int new_index, int length, uchar symbol) {
    memcpy(&*table.begin() + new_index, &*table.begin() + old_index, length);
    table[new_index + length] = symbol;
}

void Tunstall::dump(int count) {
    cout << "Table size: " << table.size() << endl;
    for(int i = 0; i < count; i++) {
        int start = index[i];
        int length = lengths[i];
        int prob = probs[i];
        cout << i << " P:" << prob << "-> ";
        for(int i = start; i < start+length; i++) {
            cout <<(char) (table[i] + 65);
        }
        cout << endl;
    }
}

void Tunstall::check(int offset, std::vector<unsigned char> &word) {
    return;
    //check tree is consistent with table.
    for(int k = 0; k < probabilities.size(); k++) {
        int node = offset + k;
        word.push_back(probabilities[k].symbol);
        int id = tree[node].first_child;
        if(id >= 0) { //leaf
            assert(word.size() == lengths[id]);
            for(int j = 0; j < word.size(); j++)
                assert(table[index[id] + j] == word[j]);
        } else {
            check(-id, word);

        }
        word.pop_back();
    }
}

void Tunstall::createDecodingTables1() {
    int n_symbols = probabilities.size();
    if(n_symbols <= 1) return;

    int dictionary_size = 1<<wordsize;
    vector<uint32_t> probs;
    index.reserve(dictionary_size*2);
    lengths.reserve(dictionary_size*2);

    //albero completo?
    table.reserve(dictionary_size*8);

    std::vector<Entry> entries; //the heap
    int count = 0;
    index.resize(n_symbols);
    lengths.resize(n_symbols);
    for(int i = 0; i < n_symbols; i++) {
        Symbol &s = probabilities[i];
        index[count] = table.size();
        lengths[count] = 1;
        table.push_back(s.symbol);
        entries.push_back(Entry(i, s.probability << 8));
        count++;
    }
    //dump(count);
    make_heap(entries.begin(), entries.end());
    int tot = n_symbols;
    while(tot < dictionary_size - n_symbols + 1) {
        Entry e = entries.front();
        std::pop_heap (entries.begin(),entries.end());
        entries.pop_back();
        //add first element reusing current index
        int pos = index[e.id]; //start of the word to be replaced
        int length = lengths[e.id];
        index[e.id] = -1;
        int start = table.size();
        table.resize(table.size() + n_symbols*length + n_symbols);


        index.resize(count + n_symbols);
        lengths.resize(count + n_symbols);
        for(int i = 0; i < n_symbols; i++) {
            Symbol &s = probabilities[i];
            //int id = i ? count++ : e.id;  //reuse last index
            int p = ((e.prob * (s.probability<<8))>>16) + 1;
            index[count] = start;
            lengths[count] = length + 1;
            append(pos, start, length, s.symbol);
            entries.push_back(Entry(count, p)); //tree.size(), p));
            push_heap(entries.begin(), entries.end());
            start += length+1;
            count++;
        }
        tot += n_symbols-1;
        //count += n_symbols - 1;
        //dump(count);
    }
    int c = 0;
    for(int i = 0; i < index.size(); i++) {
        if(index[i] != -1) { //leaf
            index[c] = index[i];
            lengths[c] = lengths[i];
            c++;
        }
    }
    //check(0, word);
    //cout << "tree.size:" << tree.size() << endl;
    //for(int i = 0; i < tree.size(); i++)
    //    cout << tree[i] << " " << endl;
    //exit(0);
}

void Tunstall::createEncodingTables1() {
        int n_symbols = probabilities.size();
        if(n_symbols <= 1) return;

        int dictionary_size = 1<<wordsize;
        vector<uint32_t> probs;
        index.reserve(dictionary_size);
        lengths.reserve(dictionary_size);

        //albero completo?
        table.reserve(dictionary_size*8);

        std::vector<Entry> entries; //the heap
        std::vector<unsigned char> word; //debug

        int count = 0;
        index.resize(n_symbols);
        lengths.resize(n_symbols);
        tree.resize(n_symbols);
        for(int i = 0; i < n_symbols; i++) {
            Symbol &s = probabilities[i];
            index[count] = table.size();
            lengths[count] = 1;
            table.push_back(s.symbol);
            tree[count] = Node(-1, count);
            entries.push_back(Entry(i, s.probability << 8));
            count++;
        }
        //dump(count);
        make_heap(entries.begin(), entries.end());
        int tot = n_symbols;
        while(tot < dictionary_size - n_symbols + 1) {
            cout << "entries: " << entries.size() << endl;
            Entry e = entries.front();
            std::pop_heap (entries.begin(),entries.end());
            entries.pop_back();
            //add first element reusing current index
            int pos = index[e.id]; //start of the word to be replaced
            int length = lengths[e.id];
            tree[e.id].first_child = count;
            int start = table.size();
            table.resize(table.size() + n_symbols*length + n_symbols);

            index.resize(count + n_symbols);
            lengths.resize(count + n_symbols);
            tree.resize(count + n_symbols);
            bool complete = true;
            for(int i = 0; i < n_symbols; i++) {
                Symbol &s = probabilities[i];
                //int id = i ? count++ : e.id;  //reuse last index
                unsigned int p = ((e.prob * (unsigned int)(s.probability<<8))>>16) + 1;
                //cout << "p: " << p << endl;


                if(0 && s.probability == 1) { //very low probability symbol. just skip.
                    tree[count]= Node(-2, count); //removed node
                    index[count] = -2;
                    lengths[count] = 0;
                    complete = false;
                } else {
                    tree[count]= Node(-1, count);
                    index[count] = start;
                    lengths[count] = length + 1;
                    append(pos, start, length, s.symbol);
                    if(p > 255) {
                        entries.push_back(Entry(count, p)); //tree.size(), p));
                        push_heap(entries.begin(), entries.end());
                    }
                    start += length+1;
                    tot++;
                }
                count++;
            }
            if(complete) { //node is completed. no need for this codeword
                //index[e.id] = -1;
            }
            //tot += n_symbols-1;
            //count += n_symbols - 1;
            //dump(count);
        }

        //export node.
/*
        //check(0, word);
        //cout << "tree.size:" << tree.size() << endl;
        //for(int i = 0; i < tree.size(); i++)
        //    cout << tree[i] << " " << endl;
        //exit(0);
        //exxport tree to bitmaps depth first.
        std::vector<int> child_count;
        std::vector<int> node_index;
        child_count.push_back(0);
        node_index.push_back(0); //this stupid tree is not rooted
        while(node_index.size()) {
            int start = node_index.back();
            int child = child_count.back();
            if(child == n_symbols) { //return up.
                bitstream.write(0, 1);
                node_index.pop_back();
                child_count.pop_back();
                continue;
            }
            int id = start + child;
            if(tree[id].first_child == -1) { //no descendants
                bitstream.write(1, 1);
                bitstream.write(0, 1);
                child_count.back()++;
                continue;
            }
            if(tree[id].first_child == -2) { //removed node]
                bitstream.write(0, 1);
                child_count.back()++;
                continue;
            }
            bitstream.write(1, 1);
            child_count.back()++; //could ne under return up, but would have to check for end.
            node_index.push_back(tree[id].first_child);
            child_count.push_back(0);
        }
        bitstream.flush();
        cout << "Number of nodes in tree: " << tree.size() << endl;
        cout << "BitStream.size in bits: " << bitstream.size*64 << endl;
*/

        int c = 0;
        for(int i = 0; i < index.size(); i++) {
            if(index[i] >= 0) { //leaf or incomplete internal node
/*                cout << c << " " ;
                for(int k = 0; k < lengths[i]; k++)
                    cout << (char)(65 + table[index[i] + k]);
                cout << endl; */
                index[c] = index[i];
                lengths[c] = lengths[i];
                //assert(tree[i] == -1);
                tree[i].index = c; //repoint to compacted index
                c++;
            } else if(index[i] == -1) { //full interna node, remove index.
                //assert(tree[i] != -1);
                //assert(tree[i] != 0);
                tree[i].index = -1;
            } else if(index[i] == -2) { //non existant node
                tree[i].index = -2;
            }
        }


}

unsigned char *Tunstall::compress1(unsigned char *data, int input_size, int &output_size) {
    int n_symbols = probabilities.size();

    if(n_symbols == 1 || input_size == 0) {
        output_size = 0;
        return NULL;
    }

    remap.resize(256, 0);
    for(int i = 0; i < n_symbols; i++) {
        Symbol &s = probabilities[i];
        remap[s.symbol] = i;
    }

    unsigned char *output = new unsigned char[input_size*2]; //use entropy here!

    assert(wordsize <= 16);
    output_size = 0;
    int input_offset = 0;
    int pos = remap[data[input_offset++]];
    int tot_length = 0;
    int old_id = 0;
    while(input_offset < input_size) {
        int new_pos = tree[pos].first_child;
        int id = tree[pos].index;

        if(new_pos == -1) {//leaf, output word
            output[output_size++] = id;
            tot_length += lengths[id];
            assert(tot_length == input_offset);
            pos = remap[data[input_offset++]];
        } else if(new_pos == -2) { //removed node, get previous internal node
            input_offset--; //backtrace
            output[output_size++] = old_id;
            tot_length += lengths[old_id];
            pos = remap[data[input_offset++]];
        } else { //internal node
            int s = remap[data[input_offset++]];
            pos = new_pos + s;
            old_id = id;
        }
    }
    assert(tot_length <= input_size);
    //end of stream can be tricky:
    //we could have a partial read (need to encode half a word)
    while(pos >= probabilities.size()) { //we didn't finish our last word.
        int new_pos = tree[pos].first_child;
        int id = tree[pos].index;

        if(id >= 0) {//end, output word
            output[output_size++] = id;
            break;
        } else {
            assert(new_pos != -2);
            //check for incomplete internal id
            pos = new_pos + data[input_offset++];
        }
    }
    assert(output_size <= input_size*2);
    return output;

}

void Tunstall::createDecodingTables() {
    sort(probabilities.begin(), probabilities.end());
    int n_symbols = probabilities.size();
    if(n_symbols <= 1) return;

    vector<deque<TSymbol> > queues(n_symbols);
    vector<unsigned char> buffer;

    //initialize adding all symbols to queues
    for(int i = 0; i < n_symbols; i++) {
        TSymbol s;
        s.probability = probabilities[i].probability;
        s.offset = buffer.size();
        s.length = 1;

        queues[i].push_back(s);
        buffer.push_back(probabilities[i].symbol);
    }
    int dictionary_size = 1<<wordsize;
    int n_words = n_symbols;
    int table_length = n_symbols;
    while(n_words < dictionary_size - n_symbols +1) {
        //find highest probability word
        int best = 0;
        float max_prob = 0;
        for(int i = 0; i < n_symbols; i++) {
            float p = queues[i].front().probability ;
            if(p > max_prob) {
                best = i;
                max_prob = p;
            }
        }

        TSymbol symbol = queues[best].front();
        //split word.
        int pos = buffer.size();
        buffer.resize(pos + n_symbols*(symbol.length + 1));
        for(int i = 0; i < n_symbols; i++) {
            TSymbol s;
            s.probability = probabilities[i].probability*symbol.probability;
            s.offset = pos;
            s.length = symbol.length + 1;

            memcpy(&buffer[pos], &buffer[symbol.offset], symbol.length);
            pos += symbol.length;
            buffer[pos++] = probabilities[i].symbol;
            queues[i].push_back(s);
        }
        table_length += (n_symbols-1)*(symbol.length + 1) +1;
        n_words += n_symbols -1;
        queues[best].pop_front();
    }
    index.clear();
    lengths.clear();
    table.clear();

    //build table and index
    index.resize(n_words);
    lengths.resize(n_words);
    table.resize(table_length);
    int word = 0;
    int pos = 0;
    for(int i = 0; i < queues.size(); i++) {
        deque<TSymbol> &queue = queues[i];
        for(int k = 0; k < queue.size(); k++) {
            TSymbol &s = queue[k];
            index[word] = pos;
            lengths[word] = s.length;
            word++;
            memcpy(&table[pos], &buffer[s.offset], s.length);
            pos += s.length;
        }
    }
    assert(index.size() <= dictionary_size);
}

void Tunstall::createEncodingTables() {
    int n_symbols = probabilities.size();
    if(n_symbols <= 1) return; //not much to compress
    //we need to reverse the table and index
    int lookup_table_size = 1;
    for(int i = 0; i < lookup_size; i++)
        lookup_table_size *= n_symbols;

    remap.resize(256, 0);
    for(int i = 0; i < n_symbols; i++) {
        Symbol &s = probabilities[i];
        remap[s.symbol] = i;
    }

    offsets.clear();
    offsets.resize(lookup_table_size, 0xffffff); //this is enough for quite large tables.
    for(int i = 0; i < index.size(); i++) {
        int low, high;
        int offset = 0;
        int table_offset = 0;
        while(1) {
            wordCode(&table[index[i] + offset], lengths[i] - offset, low, high);
            if(lengths[i] - offset <= lookup_size) {
                for(int k = low; k < high; k++)
                    offsets[table_offset + k] = i;
                break;
            }

            //word is too long
            //check if some other word already did this:
            if(offsets[table_offset + low] == 0xffffff) { //add
                offsets[table_offset + low] = -offsets.size();
                offsets.resize(offsets.size() + lookup_table_size, 0xffffff);
            }

            table_offset = -offsets[table_offset + low];
            offset += lookup_size;
        }
    }

}

unsigned char *Tunstall::compress(unsigned char *data, int input_size, int &output_size) {
    if(probabilities.size() == 1) {
        output_size = 0;
        return NULL;
    }
    unsigned char *output = new unsigned char[input_size*2]; //use entropy here!

    assert(wordsize <= 16);
    output_size = 0;
    int input_offset = 0;
    int word_offset = 0;
    int offset = 0;
    while(input_offset < input_size) {
        //read lookup_size symbols and compute code:
        int d = input_size - input_offset;
        if(d > lookup_size)
            d = lookup_size;
        int low, high;
        wordCode(data + input_offset, d, low, high);
        offset = offsets[-offset + low];
        assert(offset != 0xffffff);
        if(offset >= 0) { //ready to ouput a symbol
            output[output_size++] = offset;
            input_offset += lengths[offset] - word_offset;
            offset = 0;
            word_offset = 0;
        } else {
            word_offset += lookup_size;
            input_offset += lookup_size;
        }
    }
    //end of stream can be tricky:
    //we could have a partial read (need to encode half a word)
    if(offset < 0) {
        while(offset < 0)
            offset = offsets[-offset];
        output[output_size++] = offset;
    }
    assert(output_size <= input_size*2);
    return output;
}

void Tunstall::decompress(unsigned char *data, int input_size, unsigned char *output, int output_size) {
    unsigned char *end_output = output + output_size;
    unsigned char *end_data = data + input_size -1;
    if(probabilities.size() == 1) {
        memset(output, probabilities[0].symbol, output_size);
        return;
    }

    while(data < end_data) {
        int symbol = *data++;
        int start = index[symbol];
        int length = lengths[symbol];
        memcpy(output, &table[start], length);
        output += length;
    }

    //last symbol might override so we check.
    int symbol = *data;
    int start = index[symbol];
    int length = (end_output - output);
    memcpy(output, &table[start], length);
}

int Tunstall::decompress(unsigned char *data, unsigned char *output, int output_size) {
    unsigned char *end_output = output + output_size;
    unsigned char *start = data;
    if(probabilities.size() == 1) {
        memset(output, probabilities[0].symbol, output_size);
        return 0;
    }

    while(1) {
        int symbol = *data++;
        assert(symbol < index.size());
        int start = index[symbol];
        int length = lengths[symbol];
        if(output + length >= end_output) {
            length = (end_output - output);
            memcpy(output, &table[start], length);
            break;
        } else {
            memcpy(output, &table[start], length);
            output += length;
        }
    }
    return data - start;
}


float Tunstall::entropy() {
    float e = 0;
    for(int i = 0; i < probabilities.size(); i++) {
        float p = probabilities[i].probability;
        e += p*log(p)/log(2);
    }
    return -e;
}

/*
void BiTunstall::createDecodingTables() {
  float threshold = 0.5f/(1<<wordsize);
  //go in the probability table and remove symbols with very low probability.
  outliers.clear();
  outliers.resize(255, 0);
  special = -1;
  float totp = 0.0f;
  while(probabilities.back().probability < threshold) {
    outliers[probabilities.back().symbol] = 1;
    totp += probabilities.back().probability;
    probabilities.pop_back();
    special = probabilities.back().symbol;
  }

  if(special >= 0) {
    Symbol s;
    s.symbol = special;
    s.probability = totp;
    probabilities.push_back()
  }
  Tunstall::createDecodingTables();
}

unsigned char *BiTunstall::compress(unsigned char *data, int input_size, int &output_size) {
  if(probabilities.size() == 1) {
    output_size = 0;
    return NULL;
  }
  unsigned char *output = new unsigned char[input_size*2]; //use entropy here!

  assert(wordsize <= 16);
  output_size = 0;
  int input_offset = 0;
  int word_offset = 0;
  int offset = 0;

  while(input_offset < input_size) {
    //read lookup_size symbols and compute code:
    int d = input_size - input_offset;
    if(d > lookup_size)
      d = lookup_size;
    int low, high;
    wordCode(data + input_offset, d, low, high);
    offset = offsets[-offset + low];
    assert(offset != 0xffffff);
    if(offset >= 0) { //ready to ouput a symbol
      output[output_size++] = offset;
      input_offset += lengths[offset] - word_offset;
      offset = 0;
      word_offset = 0;
    } else {
      word_offset += lookup_size;
      input_offset += lookup_size;
    }
  }
  //end of stream can be tricky:
  //we could have a partial read (need to encode half a word)
  if(offset < 0) {
    while(offset < 0)
      offset = offsets[-offset];
    output[output_size++] = offset;
  }
  assert(output_size < input_size*2);
  return output;
}

void BiTunstall::decompress(unsigned char *data, int input_size, unsigned char *output, int output_size);
int  BiTunstall::decompress(unsigned char *data, unsigned char *output, int output_size);
*/

#endif //EXPERIMENTAL
