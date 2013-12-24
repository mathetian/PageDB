#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define MAX_BUCKETS 512          /* the index size and the maximum number of buckets */
#define SIZE 10                  /* the number of records per bucket                 */

typedef struct{
  char key_field[21];
}rec_tag;

typedef struct buck_tag{
  long number_of_records;
  int bucket_bits;
  rec_tag record[SIZE];
}BUCKET;

typedef struct{
  int bits_used_in_index;
  long buckets_used;
}head_tag;

int hasher(char*key)
{
	int rs = 0;
	return rs;
}

int makesub(char *in, int depth)
{
	char key[21];
	int i;
	int hash, sub = 0;
	int remainder;
	strncpy(key, in, 20);
	key[20] = ' ';
	hash = hasher(key)
	for(i=0; i < depth; i++)
	{
	  sub = sub << 1;
	  remainder = hash%2;
	  sub = sub + remainder;
	/*    sets up hash to look at the next lowest bit 
	     during the next iteration  */
	  hash = hash >> 1;
	}
	return sub;
}

int main()
{
  int  depth, bits;
  int  i , limit;
  int  sub;
  int  record_found;
  long rrn;
  long address;
  int begin_new, end_new;
  long bucket_address;
  long bucket_table[MAX_BUCKETS];   /* this will hold actual byte offsets rather than rrn */
  long index_size;
  long positions_used;
  
  BUCKET bucket;
  BUCKET new_bucket;
  rec_tag record;
  head_tag header_record;

  FILE *buckets;
  FILE *index;

  index = fopen("c:\\index.dat", "rb+");  /* access will only be sequential */
  buckets = fopen("c:\\buckets.dat", "rb+");  /* access will be relative     */

  fread(&header_record, sizeof(header_record), 1, index);
  index_size = pow(2, header_record.bits_used_in_index);
  fread(bucket_table, sizeof(long), index_size, index);

  /*  at this point would be a call to a function that would fill the 
      structure called record with valid data                               */
          
  sub = makesub(record.key_field, header_record.bits_used_in_index);

  fseek(buckets, bucket_table[sub], SEEK_SET);
  fread(&bucket, sizeof(bucket), 1, buckets);

  for(i=0, record_found=1; i < bucket.number_of_records && record_found != 0; i++)
    record_found = strcmp(record.key_field, bucket.record[i].key_field);

  if(record_found == 0)
  {
    puts("Record already exists.");
    puts("Enter another record.");
  }
  else
  {
    if(bucket.number_of_records < SIZE)
    {
      bucket.number_of_records += 1;
      memcpy(&bucket.record[bucket.number_of_records - 1], &record, sizeof(record));
      fseek(buckets, bucket_table[sub], SEEK_SET);
      fwrite(&bucket, sizeof(bucket), 1, buckets);
    }
    else /* the bucket is full and must be split  */
    {
      if(bucket.bucket_bits == header_record.bits_used_in_index) 
          header_record.bits_used_in_index = 
                                doublidx(header_record.bits_used_in_index, bucket_table);
      moverecs(&bucket, &new_bucket);
      header_record.buckets_used += 1;
      fseek(buckets, bucket_table[sub], SEEK_SET);
      fwrite(&bucket, sizeof(bucket), 1, buckets);
      fseek(buckets, 0, SEEK_END);
      bucket_address = ftell(buckets);
      fwrite(&new_bucket, sizeof(bucket), 1, buckets);
      
     /* now comes the tricky bit.  the address of the new bucket must be
        inserted into the appropriate place(s) in the index.  first, get the
        subscript for the table entry from any key in the new bucket.       */

      sub = makesub(new_bucket.record[0].key_field, header_record.bits_used_in_index);
	  
	  rrn = bucket_table[sub];
	  limit = pow(2,header_record.bits_used_in_index);
	  
      for(i=0; i < limit; i++)
      {
        if(bucket_table[i]==rrn) break;
	  }we
	  begin_new = i;
	  for(;bucket_table[i]==rrn && i < limit; i++) end_new = i;
      begin_new = begin_new + (end_new - begin_new + 1) / 2;
     
      for(i=begin_new; i <= end_new; i++) bucket_table[i] = bucket_address;

      fseek(index, 0L, SEEK_SET);
      fwrite(&header_record, sizeof(header_record), 1, index);
      index_size = sizeof(long) * pow(2, header_record.bits_used_in_index);
      fwrite(bucket_table, index_size, 1, index);
    
     /* now that the index and bucket maintenance is done, the rest is simple
        retrieve the bucket and add the new record.                           */

      sub = makesub(record.key_field, header_record.bits_used_in_index);

      fseek(buckets, bucket_table[sub], SEEK_SET);
      fread(&bucket, sizeof(bucket), 1, buckets);

      bucket.number_of_records += 1;
      memcpy(&bucket.record[bucket.number_of_records - 1], &record, sizeof(record));
      fseek(buckets, bucket_table[sub], SEEK_SET);
      fwrite(&bucket, sizeof(bucket), 1, buckets);
    }  /*  end else */
  }  /*  end else */

  fclose(index);
  fclose(buckets);

  return 0;
}