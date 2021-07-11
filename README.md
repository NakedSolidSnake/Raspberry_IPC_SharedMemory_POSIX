<p align="center">
  <img src="https://cdn.app.compendium.com/uploads/user/e7c690e8-6ff9-102a-ac6d-e4aebca50425/bbeb190a-b93b-4d7b-bd6c-3f9928cd87d2/Image/0ff62842b17a46978cd5cee1572e0fdb/shared_memory.png">
</p>

# _Shared Memory POSIX_

## Tópicos
* [Introdução](#introdução)
* [Systemcalls](#systemcalls)
* [Implementação](#implementação)
* [posix_shm.h](posix_shmh)
* [posix_shm.c](posix_shmc)
* [launch_processes.c](#launch_processesc)
* [button_interface.h](#button_interfaceh)
* [button_interface.c](#button_interface.c)
* [led_interface.h](#led_interfaceh)
* [led_interface.c](#led_interface.c)
* [Compilando, Executando e Matando os processos](#compilando-executando-e-matando-os-processos)
* [Compilando](#compilando)
* [Clonando o projeto](#clonando-o-projeto)
* [Selecionando o modo](#selecionando-o-modo)
* [Modo PC](#modo-pc)
* [Modo RASPBERRY](#modo-raspberry)
* [Executando](#executando)
* [Interagindo com o exemplo](#interagindo-com-o-exemplo)
* [MODO PC](#modo-pc-1)
* [MODO RASPBERRY](#modo-raspberry-1)
* [Matando os processos](#matando-os-processos)
* [Conclusão](#conclusão)
* [Referência](#referência)

## Introdução
POSIX Shared Memory é uma padronização desse recurso para que fosse altamente portável entre os sistemas. Não difere tanto da Shared Memory System V, porém utiliza uma forma de implementação totalmente nova(não abordado nesse artigo). Diferente dos outros mecanismos(Semaphore e Queue) necessita de outro recurso para que a memória alocada seja compartilhada entre os outros processos.

## Systemcalls
Para utilizar a API referente a Shared Memory é necessário realizar a linkagem com a biblioteca rt

Esta função cria ou obtém uma Shared Memory
```c
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           

int shm_open(const char *name, int oflag, mode_t mode);
```

Esta função remove uma Shared Memory criada pela shm_open
```c
#include <sys/mman.h>

int shm_unlink(const char *name);
```

## Implementação
Para facilitar o uso desse mecanismo, o uso da API referente a Shared Memory POSIX é feita através de uma abstração na forma de uma biblioteca.

### *posix_shm.h*
Para o seu uso é criada uma enumeração que determina o modo de abertura dessa shared memory e uma estrutura que vai realizar a configuração dessa shared memory, como seu nome, tamanho e o modo de operação.
```c
typedef enum 
{
    write_mode,
    read_mode
} Mode;

typedef struct 
{
    int fd;
    const char *name;
    char *buffer;
    int buffer_size;
    Mode mode;
} POSIX_SHM;
```

As funções pertinentes para criar e remover a shared memory
```c
bool POSIX_SHM_Init(POSIX_SHM *posix_shm);
bool POSIX_SHM_Cleanup(POSIX_SHM *posix_shm);
```

### *posix_shm.c*
Aqui em POSIX_SHM_Init criamos a shared memory baseada no nome e seu handle é guardado. A shared memory não possui um mecanismo próprio como a Shared Memory System V, para isso ser possível é necessário o uso do mmap para que a shared memory seja visível por outros processos.
```c
bool POSIX_SHM_Init(POSIX_SHM *posix_shm)
{
    int mode;
    posix_shm->fd = shm_open(posix_shm->name, O_RDWR | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
    if(posix_shm->fd < 0)
        return false;

    if(ftruncate(posix_shm->fd , posix_shm->buffer_size) < 0)
        return false;
    

    if(posix_shm->mode == write_mode)
        mode = PROT_WRITE;
    else if(posix_shm->mode == read_mode)
        mode = PROT_READ;

    posix_shm->buffer = mmap(NULL, posix_shm->buffer_size, mode, MAP_SHARED, posix_shm->fd, 0);
    if(posix_shm->buffer < 0)
        return false;

    return true;
}
```

Aqui em POSIX_SHM_Cleanup é realizada a remoção da shared memory baseado no seu handle
```c
bool POSIX_SHM_Cleanup(POSIX_SHM *posix_shm)
{
    if(posix_shm->buffer > 0)
        munmap(posix_shm->buffer, posix_shm->buffer_size);
    if(posix_shm->fd > 0)
        shm_unlink(posix_shm->name);

    return true;
}
```

Para demonstrar o uso desse IPC, iremos utilizar o modelo Produtor/Consumidor, onde o processo Produtor(_button_process_) vai escrever seu estado interno no arquivo, e o Consumidor(_led_process_) vai ler o estado interno e vai aplicar o estado para si. Aplicação é composta por três executáveis sendo eles:
* _launch_processes_ - é responsável por lançar os processos _button_process_ e _led_process_ através da combinação _fork_ e _exec_
* _button_interface_ - é responsável por ler o GPIO em modo de leitura da Raspberry Pi e escrever o estado interno no arquivo
* _led_interface_ - é responsável por ler do arquivo o estado interno do botão e aplicar em um GPIO configurado como saída

### *launch_processes.c*

No _main_ criamos duas variáveis para armazenar o PID do *button_process* e do *led_process*, e mais duas variáveis para armazenar o resultado caso o _exec_ venha a falhar.
```c
int pid_button, pid_led;
int button_status, led_status;
```

Em seguida criamos um processo clone, se processo clone for igual a 0, criamos um _array_ de *strings* com o nome do programa que será usado pelo _exec_, em caso o _exec_ retorne, o estado do retorno é capturado e será impresso no *stdout* e aborta a aplicação. Se o _exec_ for executado com sucesso o programa *button_process* será carregado. 
```c
pid_button = fork();

if(pid_button == 0)
{
    //start button process
    char *args[] = {"./button_process", NULL};
    button_status = execvp(args[0], args);
    printf("Error to start button process, status = %d\n", button_status);
    abort();
}   
```

O mesmo procedimento é repetido novamente, porém com a intenção de carregar o *led_process*.

```c
pid_led = fork();

if(pid_led == 0)
{
    //Start led process
    char *args[] = {"./led_process", NULL};
    led_status = execvp(args[0], args);
    printf("Error to start led process, status = %d\n", led_status);
    abort();
}
```

### *button_interface.h*
Para usar a interface do botão precisa implementar essas duas callbacks para permitir o seu uso
```c
typedef struct 
{
    bool (*Init)(void *object);
    bool (*Read)(void *object);
    
} Button_Interface;
```

A assinatura do uso da interface corresponde ao contexto do botão, que depende do modo selecionado, o contexo da Shared Memory, e a interface do botão devidamente preenchida.
```c
bool Button_Run(void *object, POSIX_SHM *posix_shm, Button_Interface *button);
```

### *button_interface.c*
A implementação da interface baseia-se em inicializar o botão, inicializar a Shared Memory, e no loop alterar o conteúdo da shared memory mediante o pressionamento do botão.
```c
bool Button_Run(void *object, POSIX_SHM *posix_shm, Button_Interface *button)
{
    int state = 0;
    if(button->Init(object) == false)
        return false;

    if(POSIX_SHM_Init(posix_shm) == false)
        return false;

    while(true)
    {
        wait_press(object, button);

        state ^= 0x01;
        snprintf(posix_shm->buffer, posix_shm->buffer_size, "state = %d", state);
    }

    POSIX_SHM_Cleanup(posix_shm);
   
    return false;
}
```

### *led_interface.h*
Para realizar o uso da interface de LED é necessário preencher os callbacks que serão utilizados pela implementação da interface, sendo a inicialização e a função que altera o estado do LED.
```c
typedef struct 
{
    bool (*Init)(void *object);
    bool (*Set)(void *object, uint8_t state);
} LED_Interface;
```

A assinatura do uso da interface corresponde ao contexto do LED, que depende do modo selecionado, o contexo da Shared Memory, e a interface do LED devidamente preenchida.
```c
bool LED_Run(void *object, POSIX_SHM *posix_shm, LED_Interface *led);
```

### *led_interface.c*
A implementação da interface baseia-se em inicializar o LED, inicializar a Shared Memory, e no loop verifica se houve alteração no conteúdo da shared memory para poder alterar o seu estado interno.
```c
bool LED_Run(void *object, POSIX_SHM *posix_shm, LED_Interface *led)
{
	int state_current = 0;
	int state_old = -1;

	if(led->Init(object) == false)
		return false;

	if(POSIX_SHM_Init(posix_shm) == false)
		return false;

	while(true)
	{
		sscanf(posix_shm->buffer, "state = %d", &state_current);
		if(state_current != state_old)
		{
			led->Set(object, state_current);
			state_old = state_current;
		}

		usleep(_1ms);
	}

	POSIX_SHM_Cleanup(posix_shm);
	return false;	
}
```

## Compilando, Executando e Matando os processos
Para compilar e testar o projeto é necessário instalar a biblioteca de [hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware) necessária para resolver as dependências de configuração de GPIO da Raspberry Pi.

## Compilando
Para faciliar a execução do exemplo, o exemplo proposto foi criado baseado em uma interface, onde é possível selecionar se usará o hardware da Raspberry Pi 3, ou se a interação com o exemplo vai ser através de input feito por FIFO e o output visualizado através de LOG.

### Clonando o projeto
Pra obter uma cópia do projeto execute os comandos a seguir:

```bash
$ git clone https://github.com/NakedSolidSnake/Raspberry_IPC_SharedMemory_POSIX
$ cd Raspberry_IPC_SharedMemory_POSIX
$ mkdir build && cd build
```

### Selecionando o modo
Para selecionar o modo devemos passar para o cmake uma variável de ambiente chamada de ARCH, e pode-se passar os seguintes valores, PC ou RASPBERRY, para o caso de PC o exemplo terá sua interface preenchida com os sources presentes na pasta src/platform/pc, que permite a interação com o exemplo através de FIFO e LOG, caso seja RASPBERRY usará os GPIO's descritos no [artigo](https://github.com/NakedSolidSnake/Raspberry_lib_hardware#testando-a-instala%C3%A7%C3%A3o-e-as-conex%C3%B5es-de-hardware).

#### Modo PC
```bash
$ cmake -DARCH=PC ..
$ make
```

#### Modo RASPBERRY
```bash
$ cmake -DARCH=RASPBERRY ..
$ make
```

## Executando
Para executar a aplicação execute o processo _*launch_processes*_ para lançar os processos *button_process* e *led_process* que foram determinados de acordo com o modo selecionado.

```bash
$ cd bin
$ ./launch_processes
```

Uma vez executado podemos verificar se os processos estão rodando atráves do comando 
```bash
$ ps -ef | grep _process
```

O output 
```bash
cssouza  16871  3449  0 07:15 pts/4    00:00:00 ./button_process
cssouza  16872  3449  0 07:15 pts/4    00:00:00 ./led_process
```
## Interagindo com o exemplo
Dependendo do modo de compilação selecionado a interação com o exemplo acontece de forma diferente

### MODO PC
Para o modo PC, precisamos abrir um terminal e monitorar os LOG's
```bash
$ sudo tail -f /var/log/syslog | grep LED
```

Dessa forma o terminal irá apresentar somente os LOG's referente ao exemplo.

Para simular o botão, o processo em modo PC cria uma FIFO para permitir enviar comandos para a aplicação, dessa forma todas as vezes que for enviado o número 0 irá logar no terminal onde foi configurado para o monitoramento, segue o exemplo
```bash
echo "0" > /tmp/shared_memory_posix_fifo
```

Output do LOG quando enviado o comando algumas vezez
```bash
Apr 15 22:42:08 cssouza-Latitude-5490 LED SHARED MEMORY POSIX[1054]: LED Status: On
Apr 15 22:42:09 cssouza-Latitude-5490 LED SHARED MEMORY POSIX[1054]: LED Status: Off
Apr 15 22:42:09 cssouza-Latitude-5490 LED SHARED MEMORY POSIX[1054]: LED Status: On
Apr 15 22:42:10 cssouza-Latitude-5490 LED SHARED MEMORY POSIX[1054]: LED Status: Off
Apr 15 22:42:10 cssouza-Latitude-5490 LED SHARED MEMORY POSIX[1054]: LED Status: On
Apr 15 22:42:10 cssouza-Latitude-5490 LED SHARED MEMORY POSIX[1054]: LED Status: Off
```

### MODO RASPBERRY
Para o modo RASPBERRY a cada vez que o botão for pressionado irá alternar o estado do LED.

## Matando os processos
Para matar os processos criados execute o script kill_process.sh
```bash
$ cd bin
$ ./kill_process.sh
```

## Conclusão
POSIX Shared Memory permite que dois processos não relacionados se comuniquem compartilhando uma região de memória, de forma similar a Shared Memory System V, porém realiza esse procedimento através de mmap que não é relacionada a sua API igual aos outros mecanismos. Normalmente é usada em conjunto com o POSIX Semaphore.

## Referência
* [Link do projeto completo](https://github.com/NakedSolidSnake/Raspberry_IPC_SharedMemory_POSIX)
* [Mark Mitchell, Jeffrey Oldham, and Alex Samuel - Advanced Linux Programming](https://www.amazon.com.br/Advanced-Linux-Programming-CodeSourcery-LLC/dp/0735710430)
* [fork, exec e daemon](https://github.com/NakedSolidSnake/Raspberry_fork_exec_daemon)
* [biblioteca hardware](https://github.com/NakedSolidSnake/Raspberry_lib_hardware)

