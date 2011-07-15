#include "common.h"

char **g_buffer;
int g_size = 64,
    g_sig = 0,
    g_curpoint = 0;

size_t *g_sizes;
struct client {
	char	active, //active?
		todo;	//what to do
	char	req[LARGE],
		*current,
		*ptr,
		*end;	// if requests need to be concatenated together
	int	sz;
	int 	fd;
};

int 	g_serverfd,
	g_which;

fd_set	g_rg_fds, 
	g_wg_fds, 
	g_eg_fds;

struct client 	g_dbase[SMALL],
		*g_forsig;

jmp_buf	g_ret;

extern context_t g_lastcontext;

void done(struct client*cur) {	
	shutdown(cur->fd, SHUT_RDWR);
	cur->active = FALSE;
	cur->todo = 0;
	cur->fd = 0;
	cur->ptr = cur->req;
}

void handle_bp(int in) {
	int ix;
	for(ix = 0;ix < SMALL;ix++) {
		if(in == g_dbase[ix].fd) {
			g_dbase[ix].active = FALSE;
			g_dbase[ix].todo = 0;
			g_dbase[ix].fd = 0;
			break;
		}
	}
	g_sig = 1;
}

void doquit(){
	for(g_which = 0;g_which < SMALL;g_which++) {
		if(g_dbase[g_which].active == TRUE) {
			done(&g_dbase[g_which]);
		}
	}
	for(g_which = 0;g_which < g_size;g_which++) {
		free(g_buffer[g_which]);
	}
	free(g_sizes);
	free(g_buffer);
	db_close();
	exit(0);
}

// Associates new connection with a database entry
void newconnection(int c) {	
	socklen_t addrlen; 
	struct sockaddr addr;
	struct client*cur;
	int g_which;

	getpeername(c, &addr, &addrlen);

	for(g_which = 0;g_which < SMALL;g_which++) {	
		if(g_dbase[g_which].active == FALSE) {
			break;
		}
	}
	cur = &g_dbase[g_which];
	cur->active = TRUE;
	cur->fd = c;
	cur->todo |= READCLIENT;
	cur->ptr = cur->req;

	fcntl(c, F_SETFL, O_NONBLOCK);

	return;
}

void swrite(int fd, char*buf, int len){
	int sz = 0, new_sz;
	while(sz < len && !g_sig) {
		new_sz = write(fd, buf + sz, len - sz);
		if(new_sz == -1) {
			usleep(1000);
		} else {
			sz += new_sz;
		}
	}
	g_sig = 0;
}
void reset(struct client*t){
	t->end = t->ptr = t->req;
	t->sz = 0;
}

void format(resSet*res, output*out, int len) {
	int ix;
	memset(out, 0, sizeof(output));
	sprintf((char*)out, "[");
	for(ix = 0; ix < (len - 1); ix++) {
//		uncompress((*res)[ix].name, uncomp);
		sprintf((char*)out + strlen((char*)out), "\"%s\",", (*res)[ix].name);
	}
//	uncompress((*res)[ix].name, uncomp);
	sprintf((char*)out + strlen((char*)out), "\"%s\"]", (*res)[ix].name);
}

int sread(struct client*t) {
	int sz = 0, new_sz;
	do {
		new_sz = recv(t->fd, t->req + sz, LARGE - sz, 0);
		sz += new_sz;
	} while(new_sz > 0 && sz > 0 && !g_sig);
	g_sig = 0;
	t->current = t->ptr = t->req;
	return sz;
}

int clean(struct client*t) {
	// go back the null values if the size is right
	for(; (t->ptr[0] == 0 && t->sz > 0); t->ptr ++, t->sz --);
	// assign the current command
	t->current = t->ptr;
	// chop off the end
	for(;t->ptr[0] > ' ';t->ptr++);
	t->ptr[0] = 0;
	// cut the size back
	t->sz -= (t->ptr - t->current);
	return(t->sz > 0);
}

int parse(char*toparse, ctx*ret) {
	char *ptr;
	ptr = toparse + 1;
	ret->name = ptr;

	while(*ret->name && *ret->name != ':') {
		ret->name++;
	}
	if(ret->name[0] == ':') {
		ret->name[0] = 0;
		ret->name++;
	} else {
		ret->name = ptr;
		ptr = 0;
	}
	memset(ret->context, 0, sizeof(context_t));
	if(ptr) {
		strncpy(ret->context, ptr, sizeof(context_t));
	}
	return 1;
}
// Some things can be enqueued
void sendstuff() {	
	struct client*t;
	ctx c;

	for(g_which = 0;g_which < SMALL;g_which++) {	
		if(g_dbase[g_which].active == TRUE) {
			t = &g_dbase[g_which];
			
			// Reading from the client
			if(FD_ISSET(t->fd, &g_rg_fds)) {
				t->sz = sread(t);
				if(t->sz == 0) {
					done(t);
				} else {
					t->todo |= WRITECLIENT;
					t->todo &= ~READCLIENT;
				}
			}
			if(FD_ISSET(t->fd, &g_wg_fds) && t->req[0] != 0) {
				while(clean(t)){
					switch(t->current[0]) {
						case 'q':
							doquit();
							break;
						case '+':
							parse(t->current, &c);
							insert(c.context, c.name);
							break;
						case '?':
							{
								output out;
								resSet rs;
								int len;

								memset(rs, 0, sizeof(resSet));
								parse(t->current, &c);

								if(search(c.context, c.name, &rs, &len)) {
									format(&rs, &out, len);
									swrite(t->fd, out, strlen(out));
								} else {
									printf("(db) [%s] Not found: %s\n", c.context, c.name);
								}
							}
							break;
						case 's':
							printf("fuck off\n");
							break;
					}

				}
				t->todo &= ~WRITECLIENT;
				t->todo |= READCLIENT;
			}
			if(FD_ISSET(t->fd, &g_eg_fds)) {	
				done(t);
			}
		}
	}
}

void doselect() {	
	int 	hi, 
		i;	

	struct client *c;

	FD_ZERO(&g_rg_fds);
	FD_ZERO(&g_eg_fds);
	FD_ZERO(&g_wg_fds);
	FD_SET(g_serverfd, &g_rg_fds);

	hi = g_serverfd;

	for(i = 0;i < SMALL;i++) {	
		if(g_dbase[i].active == TRUE) {
			c = &g_dbase[i];

			if(c->todo & READCLIENT) {
				FD_SET(c->fd, &g_rg_fds);
			}
			if(c->todo & WRITECLIENT) {
				FD_SET(c->fd, &g_wg_fds);
			}

			FD_SET(c->fd, &g_eg_fds);
			hi = GETMAX( c->fd, hi);
		}
	}

	select(hi + 1, &g_rg_fds, &g_wg_fds, &g_eg_fds, 0);
}

void handle(int signal) {
	longjmp(g_ret, 0);
}

void start_server(int argc, char*argv[]){
	socklen_t	addrlen;

	int 	ret, 
		ix,
		port = 0;

	struct	sockaddr addr;
	struct 	sockaddr_in 	proxy, *ptr;
	struct	hostent *gethostbyaddr();

	char *progname = argv[0];

	signal(SIGPIPE, handle_bp);

	FD_ZERO(&g_rg_fds);
	FD_ZERO(&g_eg_fds);
	FD_ZERO(&g_wg_fds);

	signal(SIGSEGV, handle);

	addr.sa_family = AF_INET;
	strcpy(addr.sa_data, "somename");
	memset(g_dbase, 0, sizeof(g_dbase));
	while(argc > 0) {
		if(ISA("-P", "--port")) {
			if(--argc)  {
				port = htons(atoi(*++argv));
			}
		}
		else if(ISA("-H", "--help")) {
			printf("%s\n", progname);
			printf("\t-P --port\tWhat part to run on\n");
			exit(0);
		}

		argv++;
		if(argc)argc--;
	}
	if(!port)
	{
		port = htons(8765);	
	}

	proxy.sin_family = AF_INET;
	proxy.sin_port = port;
	proxy.sin_addr.s_addr = INADDR_ANY;
	g_serverfd = socket(PF_INET, SOCK_STREAM, 0);

	while(bind(g_serverfd, (struct sockaddr*)&proxy, sizeof(proxy)) < 0) {	
		proxy.sin_port += htons(1);
	}

	addrlen = sizeof(addr);
	ret = getsockname(g_serverfd, &addr, &addrlen);
	ptr = (struct sockaddr_in*)&addr;
	listen(g_serverfd, SMALL);

	printf("%d,%d,%d\n", ntohs(proxy.sin_port), g_size, getpid());
	fflush(0);

	g_buffer = (char**)malloc(sizeof(char*) * g_size);
	g_sizes = (size_t*)malloc(sizeof(size_t) * g_size);
	for(ix = 0; ix < g_size; ix++) {
		g_buffer[ix] = (char*)malloc(sizeof(char) * (MEDIUM + 2));
		memset(g_buffer[ix], 0, MEDIUM + 2);
	}

	//ret = daemon(0,0);
	// This is the main loop
	for(;;) {
		doselect();

		if(FD_ISSET(g_serverfd, &g_rg_fds)) {
			newconnection(accept(g_serverfd, 0, 0));
		}
		setjmp(g_ret);
		sendstuff();
	}
}
