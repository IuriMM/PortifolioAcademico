from veiculo import Veiculo
from carro import Carro
from moto import Moto 
from bicicleta import Bicicleta

class Frota:
    def __init__(self):
        self.__veiculos=[]
    
    def inserir(self,veiculo):
        if veiculo in self.__veiculos:
            self.__veiculos.remove(veiculo)
        else:
            print("Erro:Apenas objetos do tipo veiculo podem ser inseridos")
    
    def remover(self,veiculo):
        if veiculo in self.__veiculos:
            self.__veiculos.remove(veiculo)
        else:
            print("veiculo nao encontrado na frota")
    
    def remover_por_tipo(self,tipo):
        self.__veiculos=[v for v in self.__veiculos if not isinstance(v,tipo)]

    def esta_vazia(self):
        return len(sef.__veiculo)==0
    def sem_motos(self):
        return len([for v in self.__veiculos if isinstance(v,Moto)])==0
    

    
    def esta_vazia_tipo(self,tipo):



