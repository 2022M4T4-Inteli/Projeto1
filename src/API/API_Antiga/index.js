const express = require('express');
const { default: mongoose } = require('mongoose');
const app = express();

//Ler JSON

app.use(
    express.urlencoded({
        extended: true,

    }),
)

app.use(express.json())

//Rota inicial

const pessoaRoutes = require('./Router/pessoaRouter')

app.use('/pessoa', pessoaRoutes)


app.get('/', (req, res) => {

    res.json({message: "Ola!"})

})

const DB_USER  = 'Gabi-Barretto'
const DB_PASSWORD = encodeURIComponent('@Barretto123')

//mongodb+srv://Gabi-Barretto:@Barretto123@cluster-teste-api.husf6fm.mongodb.net/BDdaAPI?retryWrites=true&w=majority

//Entregar Porta


mongoose.connect(
    `mongodb+srv://${DB_USER}:${DB_PASSWORD}@cluster-teste-api.husf6fm.mongodb.net/bancodaapi?retryWrites=true&w=majority`
)
.then(() => {
    app.listen(3000)
    console.log("Conectou ao BD!")
})
.catch((err) => console.log(err))

