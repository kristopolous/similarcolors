#include "common.h"

typedef struct {
	DB **dbList;
	DB *root;
	// implicit last db
	DB *pLast;
	size_t sz, current;
} dbList;
dbList g_db;

rb_red_blk_tree* tree[BINMAX];

int unsafeCtr = 0;
context_t g_lastcontext;
// forward declarationa
void IntDest(void* a);
void IntPrint(const void* a);
void InfoPrint(void* a);
void InfoDest(void *a);
int IntComp(const void* a,const void* b);
void ixPut(cmpString fname, analSet *anl);
void ixGet(analSet*anl, cmpString**list, int *len);

DB *db_getContext(context_t toFind) {
	DBT 	key, 
		data;

	int	ret,
		dbase;

	// same context as last time
	if(toFind[0] == 0) {
		if(g_db.pLast) {
			return g_db.pLast;
		} else {
			printf("(db) No contexts created yet\n");
			// create a default context
			strcpy(toFind, "_DEFAULT_");
		}
	}
	memset (&key, 0, sizeof (key));
	memset (&data, 0, sizeof (data));

	printf("(db) [%s] Finding context\n", toFind);
	key.data = toFind;
	key.size = strlen(toFind);
	if ((ret = g_db.root->get (g_db.root, NULL, &key, &data, 0)) != 0) {
		printf("(db) [%s] Creating context\n", toFind);

		if(g_db.current > g_db.sz) {
			g_db.sz += GROWBY;
			g_db.dbList = (DB**)realloc(g_db.dbList, sizeof(DB*) * g_db.sz);
			memset(g_db.dbList + (g_db.sz - GROWBY), 0, sizeof(DB*) * GROWBY);
		}
		// this is the integer to dump into the root reference
		dbase = g_db.current;

		if ((ret = db_create (&g_db.dbList[dbase], NULL, 0)) != 0) {
			fprintf (stderr, "(db) db_create: %s\n", db_strerror (ret));
		}
		if ((ret = g_db.dbList[dbase]->open (g_db.dbList[dbase], 
					NULL,
					DATABASE, 
					NULL, 
					DB_BTREE, 
					DB_CREATE, 
					0664)) != 0) {
			g_db.dbList[dbase]->err (g_db.dbList[dbase], ret, "%s", DATABASE);
		}

		// assign the last pointer for implicit contexts
		g_db.pLast = g_db.dbList[dbase];
		strcpy(g_lastcontext, toFind);

		// increment the current pointer
		g_db.current++;

		data.data = &dbase;
		data.size = sizeof(int);
		if ((ret = g_db.root->put (g_db.root, NULL, &key, &data, 0)) != 0) {
			g_db.root->err (g_db.root, ret, "(db) DB->put");
		}
		printf("(db) [%s] Created context\n", toFind);
	} else {
		memcpy(&dbase, data.data, sizeof(int));
		printf("(db) [%s] Context number %d\n", g_lastcontext, dbase);
	}
	return g_db.dbList[dbase];
}

void db_close() {
	int ix;
	for(ix = 0; ix < g_db.sz; ix++) {
		if(g_db.dbList[ix]) {
			g_db.dbList[ix]->close (g_db.dbList[ix], 0);
		}
	}
}
void db_connect() {
	int ret, ix;
	// initialize some database handlers
	g_db.dbList = (DB**)malloc(sizeof(DB*) * GROWBY);
	memset(g_db.dbList, 0, sizeof(DB*) * GROWBY);
	g_db.sz = GROWBY;
	g_db.current = 0;
	// we have a special root db

	if ((ret = db_create (&g_db.root, NULL, 0)) != 0) {
		fprintf (stderr, "db_create: %s\n", db_strerror (ret));
	}
	if ((ret = g_db.root->open (g_db.root, 
				NULL,
				DATABASE, 
				NULL, 
				DB_BTREE, 
				DB_CREATE, 
				0664)) != 0) {
		g_db.root->err (g_db.root, ret, "%s", DATABASE);
	}

	// rb tree setup
	for(ix = 0; ix < BINMAX; ix++) {
		tree[ix] = RBTreeCreate(IntComp,IntDest,InfoDest,IntPrint,InfoPrint);
	}
}

// compress the search set
void compress(char*in, cmpString out) {
	strcpy(out, in);
	out[strlen(out) - 4] = 0;
	printf("%s\n",out);
}
void uncompress(cmpString in, char*out){
	memcpy(out, in, strlen(in));
	strcpy(out + strlen(in), ".jpg");	
}

int insert(context_t context, char*fname) {
	int ret;

	DB* 	dbContext = db_getContext(context);

	DBT 	key, 
		data;

	char *pContext;
	analSet anal = {{{0}},{0}};

	if(context[0]) {
		pContext = context;
	} else {
		pContext = g_lastcontext;
	}
	printf("(%d) [%s] Adding %s\n", ++unsafeCtr, pContext, fname);

	memset (&key, 0, sizeof (key));
	memset (&data, 0, sizeof (data));

	// analyze the file to be inserted
	analyze(fname, &anal);

	// this is the simple key value store
	//compress(fname, comp);
	key.data = fname;
	key.size = strlen(fname);
	data.data = (void*)&anal;
	data.size = sizeof(analSet);

	if ((ret = dbContext->put (dbContext, NULL, &key, &data, 0)) != 0) {
		dbContext->err (dbContext, ret, "(db)");
	}

	ixPut(fname, &anal);

	return 0;
}
int search(context_t context, char*fname, resSet*set, int*len) {
	int 	ret,
		ix;

	DB* 	dbContext = db_getContext(context);

	DBT 	key, 
		data;

	analSet 	anal;
	analTuple	toComp[MAXRES * MAXINDEX];
	cmpString 	*list[MAXRES * MAXINDEX];
	
	char *pContext;
	if(context[0]) {
		pContext = context;
	} else {
		pContext = g_lastcontext;
	}

	if(strlen(fname) < 1) {
		return 0;
	}
	memset (&key, 0, sizeof (key));
	memset (&data, 0, sizeof (data));
	
	//compress(fname, comp);
	key.data = fname;
	key.size = strlen(fname);

	// first we grab the file from the table
	if ((ret = dbContext->get (dbContext, NULL, &key, &data, 0)) != 0) {
		return 0;
	}
	memcpy(&anal, (analSet*)data.data, sizeof(analSet));
	// then we execute an index search on the image
	ixGet(&anal, list, len);

	printf ("(db) [%s] getting %d\n", pContext, *len);
	// retrieve the entries from bdb
	for(ix = 0; ix < *len; ix++) {
		key.data = list[ix];
		key.size = strlen(*list[ix]);

		if ((ret = dbContext->get (dbContext, NULL, &key, &data, 0)) != 0) {
			return 0;
		} else {
			memcpy(&toComp[ix].set, (analSet*)data.data, sizeof(analSet));
			toComp[ix].name = list[ix];
		}
	}

	// then do a full analysis over the list
	// and return it
	return match(&anal, toComp, set, *len);
}

// red-black index searching
void ixPut(cmpString fname, analSet *anl) {
	int ix;
	char *nName = (char*)malloc(sizeof(cmpString));
	float *nFloat = (float*)malloc(sizeof(float) * MAXINDEX);

	strcpy(nName, fname);
	for(ix = 0; ix < MAXINDEX; ix++) {
	//	printf("(rb) %d\n", anl->indexes[ix]);
		nFloat[ix] = ((float*)anl->data)[anl->indexes[ix]];
		RBTreeInsert(	tree[anl->indexes[ix]],
				(void*)&nFloat[ix],
				(void*)nName);
	}
}
void ixGet(analSet*anl, cmpString**list, int *len){
	int ix, iz, mylen = 0;
	
	rb_red_blk_tree*ptree;
	rb_red_blk_node*pnode;

	for(ix = 0; ix < MAXINDEX; ix++) {
		ptree = tree[anl->indexes[ix]];
		//printf("(rb) %d\n", anl->indexes[ix]);
		pnode = ptree->root->left;

		while(pnode->right != ptree->nil) {
			pnode = pnode->right;
		}
		for(iz = 0; iz < MAXRES; iz++) {
			if(pnode->info) {
				list[mylen] = (void*)pnode->info;
		//		printf("(rb) %s\n", *list[mylen]);
				mylen++;
				pnode = TreePredecessor(ptree, pnode);

				// end of predecessing
				if(pnode == ptree->nil) {
					break;
				}
			} else {
				break;
			}
		}
	}
	*len = mylen;
}

void IntDest(void* a) {
  free((float*)a);
}


void IntPrint(const void* a) {
	printf("%f", *(float*)a);
}

void InfoPrint(void* a) {
	printf("%s", (char*)a); 
}

void InfoDest(void *a){
  ;
}

int IntComp(const void* a,const void* b) {
	if( *((float*)a) > *((float*)b)) {
		return(1);
	}
	if( *((float*)a) < *((float*)b)) {
		return(-1);
	}
	return(0);
}
