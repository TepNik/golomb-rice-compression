#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

void get_parameters(int argc, char *argv[],
                    int &k, bool &is_signed,
                    int &filename_ind,
                    int &output_filename_ind,
                    int &int_size,
                    bool &compress);

void write_next_bit(std::ofstream &fout, char &ch, int &ind, bool val);

template <typename type>
void write_n_th_bit(type &ch, int ind, bool val);

template <typename type>
bool get_n_th_bit(type ch, int ind);

template <typename type>
int get_overflow(type x, int k);

template <typename type>
int get_first_k_bit(type x, int k);

template <typename type>
void golomb_c(std::ifstream &fin, std::ofstream &fout, bool is_signed, int k);

void decompress(std::ifstream &fin, std::ofstream &fout);

bool get_next_bit(std::ifstream &fin, char &ch, int &ind);

template <typename type>
void golomb_d(std::ifstream &fin, std::ofstream &fout, bool is_signed, int k);

std::ifstream::pos_type filesize(const char* filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.tellg();
}

struct header
{
    unsigned char k, int_size;
    bool is_signed;
};

int main(int argc, char *argv[])
{
    std::cout << "Compressing algorithm Golomb-Rice\n";

    int k = -1,
        input_ind = -1, output_ind = -1,
        int_size = 32;
    bool is_signed = true, compress = true;
    get_parameters(argc, argv, k, is_signed, input_ind, output_ind, int_size, compress);
    if (input_ind == -1)
    {
        std::cout << "\n\nError!\n\nThere must be one input file!\n";
        return 0;
    }
    if (k == -1)
        k = int_size/2;

    std::ifstream fin;
    fin.open(argv[input_ind], std::ifstream::binary);
    std::string input_name = argv[input_ind];
    int source_size = filesize(argv[input_ind]);

    std::ofstream fout;
    std::string output_name("output");
    if (output_ind == -1)
    {
        if (compress)
            output_name = input_name + ".golomb";
        else if (input_name.find(".golomb") != std::string::npos)
            output_name = input_name.substr(0, input_name.find(".golomb"));
        fout.open(output_name, std::ifstream::binary);
    }
    else
    {
        fout.open(argv[output_ind], std::ifstream::binary);
        output_name = argv[output_ind];
    }

    if (int_size == 8 && compress)
        golomb_c<int8_t>(fin, fout, is_signed, k);
    else if (int_size == 16 && compress)
        golomb_c<int16_t>(fin, fout, is_signed, k);
    else if (int_size == 32 && compress)
        golomb_c<int32_t>(fin, fout, is_signed, k);
    else if (int_size == 64 && compress)
        golomb_c<int64_t>(fin, fout, is_signed, k);
    else if (!compress)
        decompress(fin, fout);

    int output_size = filesize(output_name.c_str());
    std::cout << "Input file = " << argv[input_ind] << '\n'
              << "Output file = " << output_name << "\n\n";

    if (compress)
        std::cout << "Compressing " << source_size << " bytes.\n"
                  << "Output size is " << output_size << "\n"
                  << "With ratio " << source_size*1./output_size << ".\n";
    else
        std::cout << "Decompressing " << source_size << " bytes.\n"
                  << "Output size is " << output_size << ".\n";

    return 0;
}

void get_parameters(int argc, char *argv[],
                    int &k, bool &is_signed,
                    int &filename_ind,
                    int &output_filename_ind,
                    int &int_size,
                    bool &compress)
{
    for (int i = 1; i < argc; ++i)
    {
        if (strstr(argv[i], "-k=") == argv[i])
        {
            k = atoi(argv[i] + 3);
        }
        else if (strstr(argv[i], "-sign=") == argv[i])
        {
            is_signed = (strcmp("true", argv[i] + 6) == 0);
        }
        else if (strstr(argv[i], "-o") == argv[i])
        {
            output_filename_ind = ++i;
            continue;
        }
        else if (strstr(argv[i], "-i") == argv[i])
        {
            int_size = atoi(argv[i] + 2);
            if (int_size != 8 && int_size != 16 &&
                int_size != 32 && int_size != 64)
            {
                std::cout << "\n\nError!\n\nSize of int must be only 8, 16, 32 or 64 bit\n";
                exit(0);
            }
        }
        else if (strcmp("-d", argv[i]) == 0)
            compress = false;
        else if (filename_ind == -1)
            filename_ind = i;
        else
        {
            std::cout << "\n\nError!\n\nThere must be only one input file.\n";
            exit(0);
        }
    }
}

void write_next_bit(std::ofstream &fout, char &ch, int &ind, bool val)
{
    write_n_th_bit(ch, ind, val);
    if (ind == 7)
    {
        ind = 0;
        fout.put(ch);
        ch = 0;
    }
    else
        ++ind;
}

template <typename type>
void write_n_th_bit(type &ch, int ind, bool val)
{
    type mask = (1 << ind);
    if (val)
        ch |= mask;
    else if (ind < sizeof(type)*8)
        ch &= ~mask;
}

template <typename type>
bool get_n_th_bit(type ch, int ind)
{
    type mask = (1 << ind);
    return (ch & mask) != 0;
}

template <typename type>
int get_overflow(type x, int k)
{
    return x / (type(1) << k);
}

template <typename type>
int get_first_k_bit(type x, int k)
{
    type mask = 0;
    for (int i = 0; i < k; ++i)
        mask |= (type(1) << i);
    return x & mask;
}

template <typename type>
void golomb_c(std::ifstream &fin, std::ofstream &fout, bool is_signed, int k)
{
    union begining
    {
        header h;
        char header_ch[sizeof(header)];
    } beg;
    beg.h.int_size = sizeof(type)*8;
    beg.h.is_signed = is_signed;
    beg.h.k = k;
    fout.write(beg.header_ch, sizeof(header));
    union{
        type x;
        char ch[sizeof(type)];
    };
    char write_ch = 0;
    int ind = 0;
    while(fin)
    {
        fin.read(ch, sizeof(type));
        if (!fin)
        {
            if (ind != 0)
                fout.put(write_ch);
            return;
        }
        if(is_signed)
            write_next_bit(fout, write_ch, ind, x < 0);
        x = std::abs(x);

        int overflow = get_overflow(x, k);
        for (int i = 0; i < overflow; ++i)
            write_next_bit(fout, write_ch, ind, 0);
        write_next_bit(fout, write_ch, ind, 1);

        for (int i = k - 1; i >= 0; --i)
            write_next_bit(fout, write_ch, ind, get_n_th_bit(x, i));
    }
    if (ind != 0)
        fout.put(write_ch);
}

void decompress(std::ifstream &fin, std::ofstream &fout)
{
    union begining
    {
        header h;
        char header_ch[sizeof(header)];
    } beg;
    fin.read(beg.header_ch, sizeof(header));

    if (beg.h.int_size == 8)
        golomb_d<int8_t>(fin, fout, beg.h.is_signed, beg.h.k);
    else if (beg.h.int_size == 16)
        golomb_d<int16_t>(fin, fout, beg.h.is_signed, beg.h.k);
    else if (beg.h.int_size == 32)
        golomb_d<int32_t>(fin, fout, beg.h.is_signed, beg.h.k);
    else if (beg.h.int_size == 64)
        golomb_d<int64_t>(fin, fout, beg.h.is_signed, beg.h.k);
    else
    {
        std::cout << "\nError!\n\nWrong input file for decompression\n";
        exit(0);
    }
}

bool get_next_bit(std::ifstream &fin, char &ch, int &ind)
{
    bool result = get_n_th_bit(ch, ind);
    if (ind == 7)
    {
        ind = 0;
        fin.get(ch);
    }
    else
        ++ind;
    return result;
}

template <typename type>
void golomb_d(std::ifstream &fin, std::ofstream &fout, bool is_signed, int k)
{
    int ind = 0;
    char ch;
    fin.get(ch);
    int count = 0;
    while(fin)
    {
        bool sign = get_next_bit(fin, ch, ind);
        int overflow = 0;
        while(get_next_bit(fin, ch, ind) == 0 && fin)
            ++overflow;
        if (!fin)
            return;
        overflow <<= k;
        union{
            type x;
            char write_ch[sizeof(type)];
        };
        x = overflow;
        for (int i = k - 1; i >= 0; --i)
        {
            bool bit = get_next_bit(fin, ch, ind);
            write_n_th_bit(x, i, bit);
        }
        if (sign)
            x *= -1;
        fout.write(write_ch, sizeof(type));
    }
}
