import React, { useEffect, useState } from 'react';
import axios from 'axios';
import DynamicDisplay from './Components/DynamicDisplay';
import { Divider } from '@mui/material';

const API_URL = process.env.REACT_APP_API_URL || 'http://localhost:5001/api/data';

function AppTest() {
    const [games, setGames] = useState([]);

    useEffect(() => {
        const fetchGames = () => {
            axios.get(API_URL)
                .then(response => {
                    setGames(response.data);
                })
                .catch(error => console.error('There was an error!', error));
        };

        fetchGames();
        const intervalId = setInterval(fetchGames, 2000);

        return () => clearInterval(intervalId);
    }, []);

    return (
        <div>
            {games.map((game) => (
                <div key={game.id}>
                    <DynamicDisplay data={game.data} />
                    <Divider variant="middle" sx={{ height: 5 }} />
                </div>
            ))}
        </div>
    );
}

export default AppTest;
