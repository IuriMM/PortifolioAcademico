from carro import Carro
from moto import Moto
from bicicleta import Bicicleta

def main():
    print("===Carro===")
    carro=Carro("corola","toyota","4","Carro confortavel","4")
    carro.imprime()
    carro.emite_som()

    print("===Moto===")
    moto=Moto("Xj6","hondda","2","moto esportiva","600")
    moto.imprime()
    moto.emite_som()

    print("===Bicicleta===")
    bicicleta= Bicicleta("Caloi advanced","Caloi","2","bike esportiva","21")
    bicicleta.imprime()
    bicicleta.emite_som()

if __name__=="__main__":
    main()