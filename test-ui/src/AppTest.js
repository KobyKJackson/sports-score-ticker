import React, { useEffect, useState, useRef } from 'react';
import axios from 'axios';
import DynamicDisplay from './Components/DynamicDisplay'; // Adjust the path as needed
import { Divider } from '@mui/material';


function AppTest() {
    const [games, setGames] = useState([]);

    useEffect(() => {
        const fetchGames = () => {
            //axios.get('http://127.0.0.1:5001/api/data')
            axios.get('http://192.168.4.27:5001/api/data')
                .then(response => {
                    setGames(response.data);
                })
                .catch(error => console.error('There was an error!', error));
        };

        const intervalId = setInterval(fetchGames, 2000); // Update every 200ms

        return () => clearInterval(intervalId);
    }, []);

    const renderGame = (game) => {
        console.log(game)
        return <div>
            <DynamicDisplay data={game.data} />
            <Divider
                variant="middle"
                sx={{
                    height: 5
                }}></Divider>
        </div>
    };

    return (
        <div>
            {games.map(renderGame)}
        </div>
    );
}

export default AppTest;
