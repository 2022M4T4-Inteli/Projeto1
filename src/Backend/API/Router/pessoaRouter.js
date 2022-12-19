const router = require('express').Router()
const Pessoa = require('../Models/Pessoa');


router.post('/', async (req, res) => {

    const {RFID, distancia, estado} = req.body

    // cria um novo objeto do tipo Date
    const date = new Date();

    // pega a hora e minuto da requisição que foi feita
    const hour = date.getHours();
    const minute = date.getMinutes();

    // formata a hora e o minuto
    let horario = `${hour}:${minute}`;


    // se as informações necessárias não forem enviadas exibe erro
    if(!RFID){
        res.status(422).json({error: 'RFID necessário!'})
    }
    if(!distancia){
        res.status(422).json({error: 'Distancia necessário!'})
    }

    // cria o objeto que contém as informações da batida do RFID
    const pessoa = {
    
        RFID,
        distancia,
        estado,
        horario

    }

    // pega o tempo em minutos
    pessoa.distancia = (pessoa.distancia/2.77)/60;

    // insere o objeto criado no banco de dados
    try {

        await Pessoa.create(pessoa)

        res.status(201).json({message: 'RFID inserida com sucesso!'})

    } catch(error) {
        res.status(500).json({ error: error })
    }

})

// requisição get para leitura das batidas com estado 2
router.get('/', async(req, res) => {
    
    try{
        const people = await Pessoa.find({ estado: "2" })
        res.status(200).json(people)
    } catch (error) {
        res.status(500).json({ error: error })
    }
})


module.exports = router