//importação das bibliotecas
const express = require('express');
const { default: mongoose } = require('mongoose');
const app = express();

// ler JSON
app.use(
    express.urlencoded({
        extended: true,

    }),
)

app.use(express.json())

//rota inicial
const pessoaRoutes = require('./Router/pessoaRouter')

app.use('/pessoa', pessoaRoutes)

//rota para exibir o html na tela
app.get('/', (req, res) => {

    //res.json({message: "Sistema iniciado!"})
    res.sendFile(__dirname + '/views/toten.html')

})

//configurações para o banco de dados
const DB_USER  = 'Gabi-Barretto'
const DB_PASSWORD = encodeURIComponent('@Barretto123')

//mongodb+srv://Gabi-Barretto:@Barretto123@cluster-teste-api.husf6fm.mongodb.net/BDdaAPI?retryWrites=true&w=majority


//conexão ao banco de dados
mongoose.connect(
    `mongodb+srv://${DB_USER}:${DB_PASSWORD}@cluster-teste-api.husf6fm.mongodb.net/bancodaapi?retryWrites=true&w=majority`
)
.then(() => {
    app.listen(3000)
    console.log("Conectou ao BD!")
})
.catch((err) => console.log(err))

