const router = require('express').Router()
const Pessoa = require('../Models/Pessoa');


router.post('/', async (req, res) => {

    const {RFID, distancia} = req.body

    if(!RFID){
        res.status(422).json({error: 'RFID necessário!'})
    }
    if(!distancia){
        res.status(422).json({error: 'Distancia necessário!'})
    }

    const pessoa = {
    
        RFID,
        distancia
    }

    try {

        await Pessoa.create(pessoa)

        res.status(201).json({message: 'RFID inserida com sucesso!'})

    } catch(error) {
        res.status(500).json({ error: error })
    }

})

module.exports = router